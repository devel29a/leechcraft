import QtQuick 2.3
import "."

Rectangle {
    id: rootRect

    signal linkActivated(string id)
    signal albumPreviewRequested(int idx)

    signal bookmarkArtistRequested(string id, string page, string tags)
    signal previewRequested(string artist)
    signal browseInfo(string artist)

    Item {
        id: bioViewContainer

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: parent.width / 1.618

        BioView {
            anchors.fill: parent
            onLinkActivated: rootRect.linkActivated(id)
            onAlbumPreviewRequested: rootRect.albumPreviewRequested(idx)
        }
    }

    Item {
        anchors.top: parent.top
        anchors.left: bioViewContainer.right
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        SimilarView {
            anchors.fill: parent
            onBookmarkArtistRequested: rootRect.bookmarkArtistRequested(id, page, tags)
            onPreviewRequested: rootRect.previewRequested(artist)
            onBrowseInfo: rootRect.browseInfo(artist)
        }
    }
}
