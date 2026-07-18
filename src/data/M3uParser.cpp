#include "M3uParser.h"
#include "ContentTypeDetector.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QMap>

namespace Data {

ParseResult M3uParser::parseFile(const QString& filePath, int playlistId) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return ParseResult();
    }
    
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();
    
    return parse(content, playlistId);
}

QString M3uParser::detectSeriesName(const QString& title) {
    // Patterns that mark a series episode. The series name is whatever comes
    // BEFORE the first episode marker:
    //   "Loki S01E02"            -> "Loki"
    //   "Loki S01 E02"           -> "Loki"
    //   "Bachelor.Point.S05E101" -> "Bachelor Point"
    //   "Dark Season 2 Episode 1"-> "Dark"
    //   "Friends 5x12"           -> "Friends"
    //   "Some Show EP 07"        -> "Some Show"
    static const QRegularExpression markers[] = {
        QRegularExpression(R"((.+?)[\s._-]+S\d{1,2}[\s._-]*E\d{1,4}\b)",
                           QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(R"((.+?)[\s._-]+Season[\s._-]*\d{1,2}\b)",
                           QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(R"((.+?)[\s._-]+\d{1,2}x\d{1,4}\b)",
                           QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(R"((.+?)[\s._-]+EP?[\s._-]*\d{1,4}\b)",
                           QRegularExpression::CaseInsensitiveOption),
    };

    for (const auto& re : markers) {
        const auto m = re.match(title);
        if (!m.hasMatch()) continue;

        QString name = m.captured(1);
        // Strip leading tags like "[Fibwatch.Com]" and normalize separators
        static const QRegularExpression bracketTag(R"(^\s*[\[(][^\])]*[\])]\s*)");
        name.remove(bracketTag);
        name.replace(QRegularExpression(R"([._-]+)"), " ");
        name = name.simplified();

        // A plausible series name: non-empty and not itself just a number
        static const QRegularExpression onlyDigits(R"(^\d+$)");
        if (!name.isEmpty() && !onlyDigits.match(name).hasMatch()) {
            return name;
        }
    }
    return QString();
}

ParseResult M3uParser::parse(const QString& m3uContent, int playlistId) {
    ParseResult result;
    QMap<QString, int> groupMap; // map group name to groupId
    int nextGroupId = 1;
    
    QStringList lines = m3uContent.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
    
    Domain::Channel currentChannel;
    currentChannel.playlistId = playlistId;
    QString currentGroupName;
    bool inChannelInfo = false;
    
    // Regular expressions for attributes
    QRegularExpression tvgIdRegex(R"re(tvg-id="([^"]*)")re");
    QRegularExpression tvgNameRegex(R"re(tvg-name="([^"]*)")re");
    QRegularExpression tvgLogoRegex(R"re(tvg-logo="([^"]*)")re");
    QRegularExpression tvgShiftRegex(R"re(tvg-shift="([^"]*)")re");
    QRegularExpression groupTitleRegex(R"re(group-title="([^"]*)")re");
    QRegularExpression httpReferrerRegex(R"re(http-referrer="([^"]*)")re", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression userAgentRegex(R"re(http-user-agent="([^"]*)")re", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression userAgentRegex2(R"re(user-agent="([^"]*)")re", QRegularExpression::CaseInsensitiveOption);
    
    for (const QString& line : lines) {
        QString tline = line.trimmed();
        
        if (tline.startsWith("#EXTINF:")) {
            inChannelInfo = true;
            currentChannel = Domain::Channel();
            currentChannel.playlistId = playlistId;
            currentChannel.type = Domain::ContentType::UNKNOWN; // Resolved once the URL is known
            
            // Extract attributes
            auto match = tvgIdRegex.match(tline);
            if (match.hasMatch()) currentChannel.tvgId = match.captured(1);
            
            match = tvgNameRegex.match(tline);
            if (match.hasMatch()) currentChannel.tvgName = match.captured(1);
            
            match = tvgLogoRegex.match(tline);
            if (match.hasMatch()) currentChannel.logoUrl = match.captured(1);
            
            match = tvgShiftRegex.match(tline);
            if (match.hasMatch()) currentChannel.tvgShift = match.captured(1);
            
            match = httpReferrerRegex.match(tline);
            if (match.hasMatch()) currentChannel.referer = match.captured(1);
            
            match = userAgentRegex.match(tline);
            if (match.hasMatch()) currentChannel.userAgent = match.captured(1);
            else {
                match = userAgentRegex2.match(tline);
                if (match.hasMatch()) currentChannel.userAgent = match.captured(1);
            }
            
            // Extract channel name (after the last comma) — needed below to
            // derive a series group when the file has no group-title
            int commaIndex = tline.lastIndexOf(',');
            if (commaIndex != -1 && commaIndex + 1 < tline.length()) {
                currentChannel.name = tline.mid(commaIndex + 1).trimmed();
            } else {
                currentChannel.name = "Unknown Channel";
            }

            QString groupName;
            match = groupTitleRegex.match(tline);
            if (match.hasMatch() && !match.captured(1).trimmed().isEmpty()) {
                // Explicit group-title in the file wins
                groupName = match.captured(1);
            } else {
                // No group-title: web-series episodes ("Loki S01E02") get
                // their own group named after the series; everything else
                // defaults to "Movie".
                const QString series = detectSeriesName(currentChannel.name);
                groupName = series.isEmpty() ? QStringLiteral("Movie") : series;
                if (!series.isEmpty()) {
                    currentChannel.type = Domain::ContentType::SERIES;
                }
            }

            // Handle group
            if (!groupMap.contains(groupName)) {
                Domain::Group newGroup;
                newGroup.id = nextGroupId++; // temporary ID, will be replaced by DB ID
                newGroup.playlistId = playlistId;
                newGroup.name = groupName;
                newGroup.orderIndex = groupMap.size();
                result.groups.push_back(newGroup);
                groupMap[groupName] = newGroup.id;
            }
            currentChannel.groupId = groupMap[groupName];
            currentGroupName = groupName;
        }
        else if (tline.startsWith("#EXTVLCOPT:http-referrer=", Qt::CaseInsensitive)) {
            int idx = tline.indexOf('=');
            if (idx != -1) currentChannel.referer = tline.mid(idx + 1).trimmed();
        } 
        else if (tline.startsWith("#EXTVLCOPT:http-user-agent=", Qt::CaseInsensitive)) {
            int idx = tline.indexOf('=');
            if (idx != -1) currentChannel.userAgent = tline.mid(idx + 1).trimmed();
        }
        else if (!tline.startsWith("#") && inChannelInfo) {
            // This is likely the stream URL
            extractHttpHeaders(tline, currentChannel);
            if (currentChannel.type != Domain::ContentType::SERIES) {
                currentChannel.type = ContentTypeDetector::detect(
                    currentChannel.name, currentChannel.streamUrl, currentGroupName);
            }
            currentChannel.orderIndex = result.channels.size();
            result.channels.push_back(currentChannel);
            inChannelInfo = false;
        }
    }
    
    return result;
}

void M3uParser::extractHttpHeaders(const QString& rawUrl, Domain::Channel& channel) {
    // Some M3Us use the format URL|Referer=...&User-Agent=...
    if (rawUrl.contains("|")) {
        QStringList parts = rawUrl.split("|");
        channel.streamUrl = parts[0].trimmed();
        
        if (parts.size() > 1) {
            QString headersPart = parts[1];
            QStringList headers = headersPart.split("&");
            for (const QString& header : headers) {
                if (header.startsWith("Referer=", Qt::CaseInsensitive)) {
                    channel.referer = header.mid(8).trimmed();
                } else if (header.startsWith("User-Agent=", Qt::CaseInsensitive)) {
                    channel.userAgent = header.mid(11).trimmed();
                }
            }
        }
    } else {
        channel.streamUrl = rawUrl;
    }
}

} // namespace Data
