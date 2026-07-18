#pragma once

#include <QString>
#include <QList>
#include "../domain/Channel.h"
#include "../domain/Group.h"
#include <vector>

namespace Data {

struct ParseResult {
    std::vector<Domain::Group> groups;
    std::vector<Domain::Channel> channels;
};

class M3uParser {
public:
    static ParseResult parse(const QString& m3uContent, int playlistId);
    static ParseResult parseFile(const QString& filePath, int playlistId);
    // Detects web-series titles like "Loki S01E02" / "Bachelor Point E101" /
    // "Dark Season 2" and returns the cleaned series name ("Loki"), or an
    // empty string if the title doesn't look like a series episode.
    static QString detectSeriesName(const QString& title);

private:
    static void extractHttpHeaders(const QString& url, Domain::Channel& channel);
};

} // namespace Data
