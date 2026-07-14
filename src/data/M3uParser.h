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

private:
    static void extractHttpHeaders(const QString& url, Domain::Channel& channel);
};

} // namespace Data
