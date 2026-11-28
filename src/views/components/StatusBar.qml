// Connection status bar shown at the top of the screens. Thin adapter: it maps
// the global `Robot` and `Session` context properties onto StatusBarView, which
// does the drawing (face icon, name, state, chips, detail panel).
// See docs/project/STATUSBAR_HOWTO.md.
import QtQuick 2.15

StatusBarView {
    id: root

    // Saved (registered) Zowi. `Session` offers load functions, not bindable
    // properties, so they are refreshed on the signals below.
    property string savedName: ""
    property string savedAddress: ""
    property string savedTransport: ""

    function refresh() {
        savedName = Session.loadActiveZowiName() || ""
        savedAddress = Session.loadActiveZowiDeviceAddress() || ""
        savedTransport = Session.loadActiveZowiTransport() || ""
    }
    Component.onCompleted: refresh()

    connected: Robot.connected
    connecting: Robot.connecting
    demo: Robot.situation === Robot.SituationDemo
    transportLost: Robot.situation === Robot.SituationTransportLost
    hasSaved: savedAddress !== ""
    name: connected ? (savedName || Robot.deviceName || "") : savedName
    mac: connected ? (Robot.deviceAddress || "") : savedAddress
    battery: Robot.battery !== undefined ? Robot.battery : -1
    lowBatteryThreshold: Robot.lowBatteryThreshold !== undefined ? Robot.lowBatteryThreshold : 50
    transport: connected
        ? (Robot.activeTransport === Robot.TransportUsb ? "usb" : "bt")
        : (savedTransport === "usb" ? "usb" : "bt")
    appId: connected ? (Robot.appId || "") : ""

    onReconnectClicked: Robot.connectToDevice(savedAddress)

    Connections {
        target: Session
        function onSessionChanged() { root.refresh() }
    }
}
