#include "M3uParser.h"
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

ParseResult M3uParser::parse(const QString& m3uContent, int playlistId) {
    ParseResult result;
    QMap<QString, int> groupMap; // map group name to groupId
    int nextGroupId = 1;
    
    QStringList lines = m3uContent.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
    
    Domain::Channel currentChannel;
    currentChannel.playlistId = playlistId;
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
            currentChannel.type = Domain::ContentType::LIVE; // Default, could be derived from group or URL later
            
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
            
            QString groupName = "Uncategorized";
            match = groupTitleRegex.match(tline);
            if (match.hasMatch()) {
                groupName = match.captured(1);
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
            
            // Extract channel name (after the last comma)
            int commaIndex = tline.lastIndexOf(',');
            if (commaIndex != -1 && commaIndex + 1 < tline.length()) {
                currentChannel.name = tline.mid(commaIndex + 1).trimmed();
            } else {
                currentChannel.name = "Unknown Channel";
            }
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
