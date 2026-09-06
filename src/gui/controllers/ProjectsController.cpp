#include "ProjectsController.h"
#include "TranslatorController.h"
#include "SessionController.h"
#include <QVariant>
#include <QDateTime>
#include <QFile>
#include <zowi/project_model.h>

ProjectsController::ProjectsController(TranslatorController *translator, SessionController *session, QObject *parent)
    : QObject(parent), m_translator(translator), m_session(session)
{
    m_projectsStore.setLogCallback([this](zowi::ProjectsStore::LogLevel level, const std::string &msg) {
        if (level == zowi::ProjectsStore::LogLevel::Warning)
            qWarning("ProjectsStore: %s", msg.c_str());
        else
            qInfo("ProjectsStore: %s", msg.c_str());
    });
    m_projectsStore.onChanged([this]() { emit projectsChanged(); });

    m_prefsStore.onChanged([this]() { emit projectsChanged(); });

    m_projectsStore.setProjectsLoader([]() {
        QFile qrc(":/projects/move.json");
        if (qrc.open(QIODevice::ReadOnly))
            return QString::fromUtf8(qrc.readAll()).toStdString();
        QFile disk("projects/move.json");
        if (disk.open(QIODevice::ReadOnly))
            return QString::fromUtf8(disk.readAll()).toStdString();
        return std::string();
    });
    m_projectsStore.loadAll();
}

QVariant ProjectsController::getProject(const QString &id) const {
    auto proj = m_projectsStore.getProject(id.toStdString());
    if (!proj) return QVariant();

    QVariantMap map;
    map["id"] = QString::fromStdString(proj->id);
    map["title"] = m_translator->translate("Project" + id.left(1).toUpper() + id.mid(1) + "Screen.qml", QString::fromStdString(proj->titleKey));
    map["description"] = m_translator->translate("Project" + id.left(1).toUpper() + id.mid(1) + "Screen.qml", QString::fromStdString(proj->descriptionKey));
    map["image"] = QString::fromStdString(proj->imageKey);
    map["url"] = m_translator->translate("Project" + id.left(1).toUpper() + id.mid(1) + "Screen.qml", QString::fromStdString(proj->urlKey));
    map["hexPath"] = QString::fromStdString(proj->hexPath);
    map["achievementId"] = QString::fromStdString(proj->achievementId);

    QVariantList questionsList;
    for (const auto &q : proj->questions) {
        QVariantMap qMap;
        qMap["text"] = m_translator->translate("Project" + id.left(1).toUpper() + id.mid(1) + "Screen.qml", QString::fromStdString(q.textKey));
        QVariantList answersList;
        for (const auto &a : q.answers) {
            QVariantMap aMap;
            aMap["text"] = m_translator->translate("Project" + id.left(1).toUpper() + id.mid(1) + "Screen.qml", QString::fromStdString(a.textKey));
            aMap["correct"] = a.correct;
            answersList.append(aMap);
        }
        qMap["answers"] = answersList;
        questionsList.append(qMap);
    }
    map["questions"] = questionsList;

    return map;
}

bool ProjectsController::isCompleted(const QString &id) const {
    std::string key = id.toStdString() + "_project_completeness";
    return m_session->getString(QString::fromStdString(key)) == "true";
}

bool ProjectsController::isQuizBlocked(const QString &id) const {
    std::string key = id.toStdString() + "_project_quiz_blockade";
    QString value = m_session->getString(QString::fromStdString(key));
    if (value.isEmpty()) return false;
    bool ok;
    long long blockadeUntil = value.toLongLong(&ok);
    if (!ok) return false;
    return QDateTime::currentMSecsSinceEpoch() < blockadeUntil;
}

int ProjectsController::getBlockadeRemainingMs(const QString &id) const {
    std::string key = id.toStdString() + "_project_quiz_blockade";
    QString value = m_session->getString(QString::fromStdString(key));
    if (value.isEmpty()) return 0;
    bool ok;
    long long blockadeUntil = value.toLongLong(&ok);
    if (!ok) return 0;
    long long now = QDateTime::currentMSecsSinceEpoch();
    long long remaining = blockadeUntil - now;
    return remaining > 0 ? static_cast<int>(remaining) : 0;
}

void ProjectsController::blockQuiz(const QString &id, int durationMs) {
    std::string key = id.toStdString() + "_project_quiz_blockade";
    long long until = QDateTime::currentMSecsSinceEpoch() + durationMs;
    m_session->saveString(QString::fromStdString(key), QString::number(until));
    emit projectsChanged();
}

void ProjectsController::setCompleted(const QString &id, bool completed) {
    std::string key = id.toStdString() + "_project_completeness";
    m_session->saveString(QString::fromStdString(key), completed ? "true" : "");
    emit projectsChanged();
}

QString ProjectsController::loadHtml(const QString &id, const QString &locale) const {
    auto read = [](const QString &path) -> QString {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return {};
        return QString::fromUtf8(f.readAll());
    };
    const QString rel = QStringLiteral("%1/%2.html").arg(id, locale);
    QString html = read(QStringLiteral("projects/") + rel);
    if (html.isEmpty())
        html = read(QStringLiteral(":/projects/") + rel);
    if (html.isEmpty() && locale != QLatin1String("en_US"))
        return loadHtml(id, QStringLiteral("en_US"));
    return html;
}

int ProjectsController::getBlockadeDurationMs() const {
    return m_prefsStore.getBlockadeDurationMs();
}

void ProjectsController::setBlockadeDurationMs(int ms) {
    m_prefsStore.setBlockadeDurationMs(ms);
    emit projectsChanged();
}

bool ProjectsController::isAchievementsEnabled() const {
    return m_prefsStore.isAchievementsEnabled();
}

void ProjectsController::setAchievementsEnabled(bool enabled) {
    m_prefsStore.setAchievementsEnabled(enabled);
    emit projectsChanged();
}

bool ProjectsController::isQuizEnabled() const {
    return m_prefsStore.isQuizEnabled();
}

void ProjectsController::setQuizEnabled(bool enabled) {
    m_prefsStore.setQuizEnabled(enabled);
    emit projectsChanged();
}