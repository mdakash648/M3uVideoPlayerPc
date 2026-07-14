#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include "../domain/Group.h"
#include "../data/ChannelRepository.h"

class GroupViewModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum GroupRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        PlaylistIdRole,
        OrderIndexRole,
        ChannelCountRole
    };

    enum SortMode {
        Ascending = 0,
        Descending = 1,
        PlaylistOrder = 2
    };
    Q_ENUM(SortMode)

    explicit GroupViewModel(Data::ChannelRepository* channelRepo, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void loadGroups(int playlistId);
    Q_INVOKABLE void setSearchQuery(const QString& query);
    Q_INVOKABLE void setSortMode(int mode);
    Q_INVOKABLE int sortMode() const { return m_sortMode; }

private:
    void applyFilterAndSort();

    Data::ChannelRepository* m_channelRepo;
    QList<Domain::Group> m_sourceGroups; // raw list as loaded from repository
    QList<Domain::Group> m_groups;       // filtered + sorted list shown to the UI
    QString m_searchQuery;
    int m_sortMode = PlaylistOrder;
};
