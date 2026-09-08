#pragma once

#include <QObject>
#include <QVariant>
#include <QString>
#include <zowi/projects_store.h>
#include <zowi/projects_preferences_store.h>
#include <zowi/translation_engine.h>

class TranslatorController;
class SessionController;

class ProjectsController : public QObject
{
    Q_OBJECT

public:
    explicit ProjectsController(TranslatorController *translator, SessionController *session, QObject *parent = nullptr);

    Q_INVOKABLE QVariant getProject(const QString &id) const;
    Q_INVOKABLE bool isCompleted(const QString &id) const;
    Q_INVOKABLE bool isQuizBlocked(const QString &id) const;
    Q_INVOKABLE int getBlockadeRemainingMs(const QString &id) const;
    Q_INVOKABLE void blockQuiz(const QString &id, int durationMs);
    Q_INVOKABLE void setCompleted(const QString &id, bool completed = true);
    Q_INVOKABLE QString loadHtml(const QString &id, const QString &locale) const;

    // Preferences
    Q_INVOKABLE int getBlockadeDurationMs() const;
    Q_INVOKABLE void setBlockadeDurationMs(int ms);
    Q_INVOKABLE bool isAchievementsEnabled() const;
    Q_INVOKABLE void setAchievementsEnabled(bool enabled);
    Q_INVOKABLE bool isQuizEnabled() const;
    Q_INVOKABLE void setQuizEnabled(bool enabled);

signals:
    void projectsChanged();

private:
    std::string projectsBundle() const;
    QString resolveUrl(const QString &rel) const;

    zowi::ProjectsStore m_projectsStore;
    zowi::ProjectsPreferencesStore m_prefsStore;
    TranslatorController *m_translator;
    SessionController *m_session;
    QString m_baseUrl;
};