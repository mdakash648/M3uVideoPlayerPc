# Implementation Plan: Channel View Model

Currently, when you click on a category/folder, the app should navigate to the `ChannelListView.qml` and show the actual channels inside that folder. Right now, it's not navigating and uses dummy data (`model: 20`).

Here is the plan to fix this:

## Proposed Changes

### 1. Create `ChannelViewModel` (C++)
I will create `ChannelViewModel` inheriting from `QAbstractListModel`.
- It will take the existing `ChannelRepository` as a dependency.
- It will expose properties like `id`, `name`, `streamUrl`, and `logoUrl` to QML.
- I will add a `Q_INVOKABLE void loadChannels(int groupId)` method so that when a folder is clicked, it queries the database for that specific folder's channels.

### 2. Update `AppController` & `CMakeLists.txt`
- Register `ChannelViewModel` in `AppController` just like we did for Playlists and Groups.
- Add the new C++ files to `CMakeLists.txt`.

### 3. Hook up Navigation in QML
- **GroupListView.qml**: Add an `onClicked` event to the category tiles to emit a signal with the `groupId` and `groupName`.
- **main.qml**: Update the `StackView` to push the `ChannelListView` when a category is clicked.
- **ChannelListView.qml**: 
  - Change `model: 20` to `model: AppController.channelViewModel`.
  - Add an `onGroupIdChanged` handler to automatically call `AppController.channelViewModel.loadChannels(groupId)`.
  - Update the UI to show the actual channel name.

## User Review Required
Please review this plan and click **Proceed** if you want me to write the C++ and QML code to connect the channels to the database and fix the navigation!
