#include "BasicGraphicsScene.hpp"

#include <queue>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include <QtWidgets/QGraphicsSceneMoveEvent>
#include <QtWidgets/QFileDialog>

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QtGlobal>

#include <QtCore/QDebug>

#include "ConnectionGraphicsObject.hpp"
#include "ConnectionIdUtils.hpp"
#include "GraphicsView.hpp"
#include "NodeGraphicsObject.hpp"


namespace QtNodes
{

    //------------------------------------------------------------------------------------------
    BasicGraphicsScene::BasicGraphicsScene(AbstractGraphModel &graphModel, QObject* parent) :
        QGraphicsScene(parent),
        _graphModel(graphModel)
    {

      connect(&_graphModel, &AbstractGraphModel::connectionCreated,
              this, &BasicGraphicsScene::onConnectionCreated);

      connect(&_graphModel, &AbstractGraphModel::connectionDeleted,
              this, &BasicGraphicsScene::onConnectionDeleted);

      connect(&_graphModel, &AbstractGraphModel::nodeCreated,
              this, &BasicGraphicsScene::onNodeCreated);

      connect(&_graphModel, &AbstractGraphModel::nodeDeleted,
              this, &BasicGraphicsScene::onNodeDeleted);

      connect(&_graphModel, &AbstractGraphModel::nodePositonUpdated,
              this, &BasicGraphicsScene::onNodePositionUpdated);

      connect(&_graphModel, &AbstractGraphModel::nodeDataChanged,
          this, &BasicGraphicsScene::onNodeDataChanged);

      connect(&_graphModel, &AbstractGraphModel::portsAboutToBeDeleted,
              this, &BasicGraphicsScene::onPortsAboutToBeDeleted);

      connect(&_graphModel, &AbstractGraphModel::portsDeleted,
              this, &BasicGraphicsScene::onPortsDeleted);

      connect(&_graphModel, &AbstractGraphModel::portsAboutToBeInserted,
              this, &BasicGraphicsScene::onPortsAboutToBeInserted);

      connect(&_graphModel, &AbstractGraphModel::portsInserted,
              this, &BasicGraphicsScene::onPortsInserted);

      setItemIndexMethod(QGraphicsScene::NoIndex);
      traverseGraphAndPopulateGraphicsObjects();
    }

    //------------------------------------------------------------------------------------------
    BasicGraphicsScene::~BasicGraphicsScene()
    {
    }

    //------------------------------------------------------------------------------------------
    AbstractGraphModel const& BasicGraphicsScene::graphModel() const
    {
      return _graphModel;
    }

    //------------------------------------------------------------------------------------------
    AbstractGraphModel& BasicGraphicsScene::graphModel()
    {
      return _graphModel;
    }

    //------------------------------------------------------------------------------------------
    std::unique_ptr<ConnectionGraphicsObject> const& BasicGraphicsScene::makeDraftConnection(ConnectionId const incompleteConnectionId)
    {
      myDraftConnection = std::make_unique<ConnectionGraphicsObject>(*this, incompleteConnectionId);
      myDraftConnection->grabMouse();
      return myDraftConnection;
    }

    //------------------------------------------------------------------------------------------
    void BasicGraphicsScene::resetDraftConnection()
    {
        myDraftConnection->hide();
    }

    //------------------------------------------------------------------------------------------
    void BasicGraphicsScene::clearScene()
    {
      auto const &allNodeIds = graphModel().allNodeIds();

      for ( auto nodeId : allNodeIds)
      {
        graphModel().deleteNode(nodeId);
      }
    }

    //------------------------------------------------------------------------------------------
    NodeGraphicsObject* BasicGraphicsScene::nodeGraphicsObject(NodeId nodeId)
    {
      NodeGraphicsObject* ngo = nullptr;
      auto it = _nodeGraphicsObjects.find(nodeId);

      if (it != _nodeGraphicsObjects.end())
      {
        ngo = it->second.get();
      }

      return ngo;
    }

    //------------------------------------------------------------------------------------------
    ConnectionGraphicsObject* BasicGraphicsScene::connectionGraphicsObject(ConnectionId connectionId)
    {
      ConnectionGraphicsObject* cgo = nullptr;
      auto it = myConnectionGraphicsObjects.find(connectionId);

      if (it != myConnectionGraphicsObjects.end())
      {
        cgo = it->second.get();
      }

      return cgo;
    }

    //------------------------------------------------------------------------------------------
    QMenu* BasicGraphicsScene::createSceneMenu(QPointF const scenePos)
    {
      Q_UNUSED(scenePos);
      return nullptr;
    }

    //------------------------------------------------------------------------------------------
    void BasicGraphicsScene::traverseGraphAndPopulateGraphicsObjects()
    {
      auto allNodeIds = _graphModel.allNodeIds();

      for (auto nodeId : allNodeIds)
      {
          _nodeGraphicsObjects[nodeId] =
              std::make_unique<NodeGraphicsObject>(*this, nodeId);
      }

      for (auto nodeId : allNodeIds)
      {
          unsigned int nOutPorts =
              _graphModel.nodeData(nodeId, NodeRole::NumberOfOutPorts).toUInt();

          for (PortIndex index = 0; index < nOutPorts; ++index)
          {
              auto connectedNodes =
                  _graphModel.connectedNodes(nodeId,
                      PortType::Out,
                      index);

              for (auto cn : connectedNodes)
              {

                  auto connectionId = std::make_tuple(nodeId, index, cn.first, cn.second);
                  if (myConnectionGraphicsObjects.find(connectionId) == myConnectionGraphicsObjects.end())
                  {
                      myConnectionGraphicsObjects[connectionId] =
                          std::make_unique<ConnectionGraphicsObject>(*this,
                              connectionId);
                  }
              }
          }
      }
    }

    //------------------------------------------------------------------------------------------
    void BasicGraphicsScene::updateAttachedNodes(ConnectionId const connectionId, PortType const portType)
    {
      auto node = nodeGraphicsObject(getNodeId(portType, connectionId));

      if (node)
      {
        node->update();
      }
    }

    //------------------------------------------------------------------------------------------
    void BasicGraphicsScene::onConnectionDeleted(ConnectionId const connectionId)
    {
        auto it = myConnectionGraphicsObjects.find(connectionId);
        if (it != myConnectionGraphicsObjects.end())
        {
            myConnectionGraphicsObjects.erase(it);
        }

        // TODO: do we need it?
        if (myDraftConnection && myDraftConnection->connectionId() == connectionId)
        {
            myDraftConnection.reset();
        }

        updateAttachedNodes(connectionId, PortType::Out);
        updateAttachedNodes(connectionId, PortType::In);
    }

    //------------------------------------------------------------------------------------------
    void BasicGraphicsScene::onConnectionCreated(ConnectionId const connectionId)
    {
      myConnectionGraphicsObjects[connectionId] = std::make_unique<ConnectionGraphicsObject>(*this, connectionId);

      updateAttachedNodes(connectionId, PortType::Out);
      updateAttachedNodes(connectionId, PortType::In);
    }

    //------------------------------------------------------------------------------------------
    void BasicGraphicsScene::onNodeDeleted(NodeId const nodeId)
    {
      auto it = _nodeGraphicsObjects.find(nodeId);
      if (it != _nodeGraphicsObjects.end())
      {
        _nodeGraphicsObjects.erase(it);
      }
    }

    //------------------------------------------------------------------------------------------
    void BasicGraphicsScene::onNodeCreated(NodeId const nodeId)
    {
        _nodeGraphicsObjects[nodeId] = std::make_unique<NodeGraphicsObject>(*this, nodeId);
    }

    //------------------------------------------------------------------------------------------
    void BasicGraphicsScene::onNodePositionUpdated(NodeId const nodeId)
    {
      auto node = nodeGraphicsObject(nodeId);
      if (node)
      {
        node->setPos(_graphModel.nodeData(nodeId, NodeRole::Position).value<QPointF>());
        node->update();
        node->moveConnections();
      }
    }

    //------------------------------------------------------------------------------------------
    void BasicGraphicsScene::onNodeDataChanged(NodeId const nodeId)
    {
        auto node = nodeGraphicsObject(nodeId);
        if (!node)
            return;
            
        node->update();
        node->moveConnections();
    }

    //------------------------------------------------------------------------------------------
    void BasicGraphicsScene::onPortsAboutToBeDeleted(NodeId const nodeId, PortType const portType, std::unordered_set<PortIndex> const &portIndexSet)
    {
      Q_UNUSED(nodeId);
      Q_UNUSED(portType);
      Q_UNUSED(portIndexSet);
      //NodeGraphicsObject * node = nodeGraphicsObject(nodeId);

      //if (node)
      //{
      //for (auto portIndex : portIndexSet)
      //{
      //auto const connectedNodes =
      //_graphModel.connectedNodes(nodeId, portType, portIndex);

      //for (auto cn : connectedNodes)
      //{
      //ConnectionId connectionId =
      //(portType == PortType::In) ?
      //std::make_tuple(cn.first, cn.second, nodeId, portIndex) :
      //std::make_tuple(nodeId, portIndex, cn.first, cn.second);

      //deleteConnection(connectionId);
      //}
      //}
      //}
    }

    //------------------------------------------------------------------------------------------
    void BasicGraphicsScene::onPortsDeleted(NodeId const nodeId, PortType const portType, std::unordered_set<PortIndex> const &portIndexSet)
    {
      Q_UNUSED(nodeId);
      Q_UNUSED(portType);
      Q_UNUSED(portIndexSet);

      NodeGraphicsObject* node = nodeGraphicsObject(nodeId);

      if (node)
      {
        node->update();
      }
    }

    //------------------------------------------------------------------------------------------
    void BasicGraphicsScene::onPortsAboutToBeInserted(NodeId const nodeId, PortType const portType, std::unordered_set<PortIndex> const &portIndexSet)
    {
      Q_UNUSED(nodeId);
      Q_UNUSED(portType);
      Q_UNUSED(portIndexSet);
    }

    //------------------------------------------------------------------------------------------
    void BasicGraphicsScene::onPortsInserted(NodeId const nodeId, PortType const portType, std::unordered_set<PortIndex> const &portIndexSet)
    {
      Q_UNUSED(nodeId);
      Q_UNUSED(portType);
      Q_UNUSED(portIndexSet);
    }
}
