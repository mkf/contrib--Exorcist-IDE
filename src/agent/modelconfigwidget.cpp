#include "modelconfigwidget.h"
#include "agent/openaicompatpresets.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QVBoxLayout>

ModelConfigWidget::ModelConfigWidget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    auto *title = new QLabel(tr("Model Configuration"), this);
    title->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 14px;"));
    layout->addWidget(title);

    // Per-mode model selectors
    const QStringList modes = {tr("Ask"), tr("Edit"), tr("Agent"), tr("Ghost Text")};
    for (const auto &mode : modes) {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel(mode + QStringLiteral(":"), this));
        auto *combo = new QComboBox(this);
        combo->setMinimumWidth(200);
        m_modeComboBoxes[mode] = combo;
        row->addWidget(combo, 1);
        layout->addLayout(row);

        connect(combo, &QComboBox::currentTextChanged, this, [this, mode](const QString &model) {
            emit modelChanged(mode, model);
        });
    }

    layout->addSpacing(12);

    // OpenAI-compatible provider preset
    auto *endpointTitle = new QLabel(tr("OpenAI-compatible Provider"), this);
    endpointTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    layout->addWidget(endpointTitle);

    auto *presetRow = new QHBoxLayout;
    presetRow->addWidget(new QLabel(tr("Provider:"), this));
    m_presetCombo = new QComboBox(this);
    for (const OpenAICompat::Preset &p : OpenAICompat::presets())
        m_presetCombo->addItem(p.displayName, p.key);
    presetRow->addWidget(m_presetCombo, 1);
    layout->addLayout(presetRow);

    auto *epRow = new QHBoxLayout;
    epRow->addWidget(new QLabel(tr("URL:"), this));
    m_endpointEdit = new QLineEdit(this);
    epRow->addWidget(m_endpointEdit, 1);
    layout->addLayout(epRow);

    auto *keyRow = new QHBoxLayout;
    keyRow->addWidget(new QLabel(tr("API Key:"), this));
    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(tr("sk-..."));
    keyRow->addWidget(m_apiKeyEdit, 1);
    layout->addLayout(keyRow);

    auto *headerRow = new QHBoxLayout;
    headerRow->addWidget(new QLabel(tr("Extra Headers:"), this));
    m_headersEdit = new QLineEdit(this);
    m_headersEdit->setPlaceholderText(QStringLiteral("X-Api-Version: 2024-01-01"));
    headerRow->addWidget(m_headersEdit, 1);
    layout->addLayout(headerRow);

    auto *saveBtn = new QPushButton(tr("Save Endpoint"), this);
    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        const QString preset = activePreset();
        {
            QSettings s;
            OpenAICompat::saveEndpoint(s, preset, m_endpointEdit->text());
            OpenAICompat::saveApiKey(s, preset, m_apiKeyEdit->text());
        }
        emit endpointConfigured(m_endpointEdit->text().trimmed(),
                                 m_apiKeyEdit->text().trimmed(),
                                 m_headersEdit->text().trimmed());
    });
    layout->addWidget(saveBtn);

    connect(m_presetCombo, &QComboBox::currentIndexChanged, this, [this]() {
        QSettings s;
        OpenAICompat::saveActivePreset(s, activePreset());
        reloadPresetFields();
    });

    // Initial state from the saved active preset.
    {
        QSettings s;
        const QString key = OpenAICompat::loadActivePreset(s);
        const int idx = m_presetCombo->findData(key);
        QSignalBlocker b(m_presetCombo);
        m_presetCombo->setCurrentIndex(
            idx >= 0 ? idx : m_presetCombo->findData(QStringLiteral("custom")));
    }
    reloadPresetFields();

    // Thinking budget
    layout->addSpacing(12);
    auto *thinkRow = new QHBoxLayout;
    thinkRow->addWidget(new QLabel(tr("Thinking Token Budget:"), this));
    m_thinkBudgetEdit = new QLineEdit(this);
    m_thinkBudgetEdit->setPlaceholderText(QStringLiteral("10000"));
    m_thinkBudgetEdit->setMaximumWidth(120);
    thinkRow->addWidget(m_thinkBudgetEdit);
    thinkRow->addStretch();
    layout->addLayout(thinkRow);

    layout->addStretch();
}

void ModelConfigWidget::setModels(const QList<ModelInfo> &models)
{
    for (auto *combo : m_modeComboBoxes) {
        combo->blockSignals(true);
        combo->clear();
        for (const auto &m : models) {
            QString label = m.name;
            if (m.capabilities.vision) label += QStringLiteral(" \U0001F441");
            if (m.capabilities.toolCalls) label += QStringLiteral(" \U0001F527");
            combo->addItem(label, m.name);
        }
        combo->blockSignals(false);
    }
}

QString ModelConfigWidget::selectedModel(const QString &mode) const
{
    auto *combo = m_modeComboBoxes.value(mode);
    return combo ? combo->currentData().toString() : QString();
}

void ModelConfigWidget::setSelectedModel(const QString &mode, const QString &model)
{
    auto *combo = m_modeComboBoxes.value(mode);
    if (!combo) return;
    const int idx = combo->findData(model);
    if (idx >= 0) combo->setCurrentIndex(idx);
}

QString ModelConfigWidget::customEndpoint() const { return m_endpointEdit->text().trimmed(); }
QString ModelConfigWidget::customApiKey() const { return m_apiKeyEdit->text().trimmed(); }
int ModelConfigWidget::thinkingBudget() const { return m_thinkBudgetEdit->text().toInt(); }

QString ModelConfigWidget::activePreset() const
{
    if (!m_presetCombo)
        return QStringLiteral("custom");
    const QString key = m_presetCombo->currentData().toString();
    return key.isEmpty() ? QStringLiteral("custom") : key;
}

void ModelConfigWidget::reloadPresetFields()
{
    const QString key = activePreset();
    QSettings s;

    if (m_endpointEdit) {
        QSignalBlocker b(m_endpointEdit);
        m_endpointEdit->setText(OpenAICompat::loadEndpoint(s, key));
    }
    if (m_apiKeyEdit) {
        QSignalBlocker b(m_apiKeyEdit);
        m_apiKeyEdit->setText(OpenAICompat::loadApiKey(s, key));
    }

    const OpenAICompat::Preset *p = OpenAICompat::presetByKey(key);
    if (m_endpointEdit) {
        const QString hint = (p && !p->defaultEndpoint.isEmpty())
            ? p->defaultEndpoint
            : QStringLiteral("https://api.example.com/v1/chat/completions");
        m_endpointEdit->setPlaceholderText(hint);
    }
}
