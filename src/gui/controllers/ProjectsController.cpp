#include "ProjectsController.h"
#include "TranslatorController.h"
#include "SessionController.h"
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QFile>
#include <zowi/project_model.h>

namespace {

// Read a file preferring the on-disk copy (dev / hot-reload) then the compiled
// Qt resource, mirroring how translations and config are resolved.
QByteArray readResource(const QString &rel) {
    QFile disk(rel);
    if (disk.open(QIODevice::ReadOnly))
        return disk.readAll();
    QFile qrc(QStringLiteral(":/projects/") + rel);
    if (qrc.open(QIODevice::ReadOnly))
        return qrc.readAll();
    return {};
}

QString readText(const QString &rel) {
    return QString::fromUtf8(readResource(rel));
}

std::optional<QJsonObject> readJsonObject(const QString &rel) {
    QByteArray data = readResource(rel);
    if (data.isEmpty())
        return std::nullopt;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return std::nullopt;
    return doc.object();
}

} // namespace

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

    if (auto index = readJsonObject(QStringLiteral("index.json")))
        m_baseUrl = index->value(QStringLiteral("base_url")).toString();
    qInfo("ProjectsController: base_url = '%s'", qPrintable(m_baseUrl));

    m_projectsStore.setProjectsLoader([this]() { return projectsBundle(); });
    m_projectsStore.loadAll();
}

// Bundle every enabled project's project.json into a single JSON array, driven
// by projects/index.json. Core stays Qt-free; resource discovery lives here.
std::string ProjectsController::projectsBundle() const {
    auto index = readJsonObject(QStringLiteral("index.json"));
    if (!index)
        return {};

    QJsonArray bundle;
    const QJsonArray ids = index->value(QStringLiteral("projects")).toArray();
    for (const auto &idVal : ids) {
        if (!idVal.isString())
            continue;
        const QString id = idVal.toString();
        auto proj = readJsonObject(id + QStringLiteral("/project.json"));
        if (proj)
            bundle.append(*proj);
        else
            qWarning("ProjectsController: missing project.json for '%s'", qPrintable(id));
    }
    return QJsonDocument(bundle).toJson(QJsonDocument::Compact).toStdString();
}

// Join a project's relative url with the configured base_url, normalising
// slashes. Absolute URLs (http(s)://, etc.) and empty values pass through.
QString ProjectsController::resolveUrl(const QString &rel) const {
    if (m_baseUrl.isEmpty() || rel.isEmpty())
        return rel;
    if (rel.contains(QStringLiteral("://")))
        return rel;

    QString base = m_baseUrl.trimmed();
    QString path = rel.trimmed();
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    while (path.startsWith(QLatin1Char('/')))
        path.remove(0, 1);
    return base + QLatin1Char('/') + path;
}

QVariant ProjectsController::getProject(const QString &id) const {
    auto proj = m_projectsStore.getProject(id.toStdString());
    if (!proj) return QVariant();

    const QString locale = m_translator->currentLocale();

    QVariantMap map;
    map["id"] = QString::fromStdString(proj->id);

    auto strings = readJsonObject(id + QStringLiteral("/strings/") + locale + QStringLiteral(".json"));
    if (!strings && locale != QLatin1String("en_US"))
        strings = readJsonObject(id + QStringLiteral("/strings/en_US.json"));

    QString titleKey = QString::fromStdString(proj->titleKey);
    QString descKey = QString::fromStdString(proj->descriptionKey);
    QString urlKey = QString::fromStdString(proj->urlKey);

    if (strings) {
        map["title"] = strings->value(QStringLiteral("title")).toString(titleKey);
        QString desc = strings->value(descKey).toString();
        if (desc.isEmpty())
            desc = strings->value(QStringLiteral("learning_description")).toString();
        map["description"] = desc.isEmpty() ? descKey : desc;
        map["url"] = resolveUrl(strings->value(QStringLiteral("url")).toString(urlKey));
    } else {
        map["title"] = m_translator->translate("Project" + id.left(1).toUpper() + id.mid(1) + "Screen.qml", titleKey);
        map["description"] = m_translator->translate("Project" + id.left(1).toUpper() + id.mid(1) + "Screen.qml", descKey);
        map["url"] = resolveUrl(m_translator->translate("Project" + id.left(1).toUpper() + id.mid(1) + "Screen.qml", urlKey));
    }

    map["image"] = QString::fromStdString(proj->imageKey);
    map["hexPath"] = QString::fromStdString(proj->hexPath);
    map["achievementId"] = QString::fromStdString(proj->achievementId);

    // Quiz content is loaded from per-locale quiz/<locale>.json (inline text).
    auto quiz = readJsonObject(id + QStringLiteral("/quiz/") + locale + QStringLiteral(".json"));
    if (!quiz && locale != QLatin1String("en_US"))
        quiz = readJsonObject(id + QStringLiteral("/quiz/en_US.json"));

    QVariantList questionsList;
    if (quiz) {
        const QJsonArray qs = quiz->value(QStringLiteral("questions")).toArray();
        for (const auto &qVal : qs) {
            QJsonObject q = qVal.toObject();
            QVariantMap qMap;
            qMap["text"] = q.value(QStringLiteral("text")).toString();
            QVariantList answersList;
            const QJsonArray ans = q.value(QStringLiteral("answers")).toArray();
            for (const auto &aVal : ans) {
                QJsonObject a = aVal.toObject();
                QVariantMap aMap;
                aMap["text"] = a.value(QStringLiteral("text")).toString();
                aMap["correct"] = a.value(QStringLiteral("correct")).toBool(false);
                answersList.append(aMap);
            }
            qMap["answers"] = answersList;
            questionsList.append(qMap);
        }
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
    const QString rel = QStringLiteral("%1/page/%2.html").arg(id, locale);
    QString html = readText(rel);
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
