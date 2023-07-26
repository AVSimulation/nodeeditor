#pragma once

#include <limits>
#include <tuple>

#include <QtCore/QMetaObject>

#include "Export.hpp"

/**
 * @file
 * Important definitions used throughout the library.
 */

namespace QtNodes
{
NODE_EDITOR_PUBLIC Q_NAMESPACE

/**
 * Constants used for fetching QVariant data from GraphModel.
 */
enum class NodeRole
{
  Type             = 0, ///< Type of the current node, usually a string.
  Position         = 1, ///< `QPointF` positon of the node on the scene.
  Size             = 2, ///< `QSize` for resizable nodes.
  CaptionVisible   = 3, ///< `bool` for caption visibility.
  Caption          = 4, ///< `QString` for node caption.
  Style            = 5, ///< Custom NodeStyle.
  NumberOfInPorts  = 6, ///< `unsigned int`
  NumberOfOutPorts = 7, ///< `unsigned int`
  Widget           = 8, ///< Optional `QWidget*` or `nullptr`
  Parameters       = 9, ///< `QString` parameters of the node on the scene.
  User             = 10, ///< QList<QVariant> user data specific to each node.
  Tooltip          = 11 ///< QString: a tooltip to display when the node is hovered (optional)
};
Q_ENUM_NS(NodeRole)


/**
 * Specific flags regulating node features and appeaarence.
 */
enum NodeFlag
{
  NoFlags   = 0x0, ///< Default NodeFlag
  Resizable = 0x1, ///< Lets the node be resizable
};

Q_DECLARE_FLAGS(NodeFlags, NodeFlag)
Q_FLAG_NS(NodeFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(NodeFlags)


/**
 * Constants for fetching port-related information from the GraphModel.
 */
enum class PortRole
{
  Data                 = 0, ///< `std::shared_ptr<NodeData>`.
  DataType             = 1, ///< `QString` describing the port data type.
  ConnectionPolicyRole = 2, ///< `enum` ConnectionPolicyRole
  CaptionVisible       = 3, ///< `bool` for caption visibility.
  Caption              = 4, ///< `QString` for port caption.
  ColorType            = 5, ///< `enum` describing the type recognized for colorization.
  User                 = 6, ///< QList<QVariant> user data specific to each port
  DefaultValue         = 7, ///< 'QString' default value for a port.
  Tooltip              = 8, ///< QString: a tooltip to display when the port is hovered (optional)
  Unit                 = 9, ///< QString: The unit of the port (empty if no unit)
  Description          = 10 ///< QString: The port description (empty if none)

};
Q_ENUM_NS(PortRole)


/**
 * Defines how many connections are possible to attach to ports. The
 * values are fetched using PortRole::ConnectionPolicy.
 */
enum class ConnectionPolicy
{
  One, ///< Just one connection for each port.
  Many, ///< Any number of connections possible for the port.
};
Q_ENUM_NS(ConnectionPolicy)


/**
 * Used for distinguishing input and output node ports.
 */
enum class PortType
{
  In   = 0, ///< Input node port (from the left).
  Out  = 1, ///< Output node port (from the right).
  None = 2
};
Q_ENUM_NS(PortType)


/// ports are consecutively numbered starting from zero.
using PortIndex = unsigned int;

static constexpr PortIndex InvalidPortIndex =
  std::numeric_limits<PortIndex>::max();

/// Unique Id associated with each node in the GraphModel.
using NodeId = unsigned int;

static constexpr NodeId InvalidNodeId =
  std::numeric_limits<NodeId>::max();

/// A unique connection identificator that stores a tuple made of (out `NodeId`, out `PortIndex`, in `NodeId`, in * `PortIndex`)
using ConnectionId = std::tuple<NodeId, PortIndex,  // Port Out
                                NodeId, PortIndex>; // Port In

//a structure to ease ports manipulations.
struct Port
{
    PortIndex portIndex;
    PortType portType;

    Port(): portIndex(InvalidPortIndex), portType(PortType::None){}

    bool operator==(const Port& other)
    {
        return portIndex == other.portIndex &&
            portType == other.portType;
    }
    bool operator!=(const Port& other)
    {
        return portIndex != other.portIndex ||
            portType != other.portType;
    }
};
}
