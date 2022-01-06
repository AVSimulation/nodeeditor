#include "NodePainter.hpp"

#include <cmath>

#include <QtCore/QMargins>

#include "AbstractGraphModel.hpp"
#include "ConnectionGraphicsObject.hpp"
#include "ConnectionIdUtils.hpp"
#include "NodeGeometry.hpp"
#include "NodeGraphicsObject.hpp"
#include "NodeState.hpp"
#include "PortType.hpp"
#include "StyleCollection.hpp"


namespace QtNodes
{

void
NodePainter::
paint(QPainter * painter,
      NodeGraphicsObject & ngo)
{
  NodeGeometry geometry(ngo);
  geometry.recalculateSizeIfFontChanged(painter->font());

  drawNodeRect(painter, ngo);

  drawConnectionPoints(painter, ngo);

  drawFilledConnectionPoints(painter, ngo);

  drawNodeCaptionBackground(painter, ngo);

  drawNodeCaption(painter, ngo);

  drawEntryLabels(painter, ngo);

  drawResizeRect(painter, ngo);
}


void
NodePainter::
drawNodeRect(QPainter * painter,
             NodeGraphicsObject &ngo)
{
  AbstractGraphModel const &model = ngo.graphModel();

  NodeId const nodeId = ngo.nodeId();

  NodeGeometry geom(ngo);
  QSize size = geom.size();

  QJsonDocument json =
    QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));

  NodeStyle nodeStyle(json);

  auto color = ngo.isSelected() ?
               nodeStyle.SelectedBoundaryColor :
               nodeStyle.NormalBoundaryColor;

  if (ngo.nodeState().hovered())
  {
    QPen p(color, nodeStyle.HoveredPenWidth);
    painter->setPen(p);
  }
  else
  {
    QPen p(color, nodeStyle.PenWidth);
    painter->setPen(p);
  }

  QLinearGradient gradient(QPointF(0.0, 0.0),
                           QPointF(2.0, size.height()));

  gradient.setColorAt(0.0,  nodeStyle.GradientColor0);
  gradient.setColorAt(0.03, nodeStyle.GradientColor1);
  gradient.setColorAt(0.97, nodeStyle.GradientColor2);
  gradient.setColorAt(1.0,  nodeStyle.GradientColor3);

  painter->setBrush(gradient);

  float diam = nodeStyle.ConnectionPointDiameter;

  QRectF boundary(-diam, -diam,
                  2.0 * diam + size.width(),
                  2.0 * diam + size.height());

  double const radius = 3.0;

  painter->drawRoundedRect(boundary, radius, radius);
}


void
NodePainter::
drawConnectionPoints(QPainter * painter,
                     NodeGraphicsObject &ngo)
{
  AbstractGraphModel const &model = ngo.graphModel();
  NodeId const nodeId     = ngo.nodeId();
  NodeGeometry geom(ngo);

  QJsonDocument json =
    QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
  NodeStyle nodeStyle(json);

  auto const &connectionStyle = StyleCollection::connectionStyle();

  float diameter       = nodeStyle.ConnectionPointDiameter;
  auto reducedDiameter = diameter * 0.6;

  for (PortType portType: {PortType::Out, PortType::In})
  {
    size_t const n =
      model.nodeData(nodeId,
                     (portType == PortType::Out) ?
                     NodeRole::NumberOfOutPorts :
                     NodeRole::NumberOfInPorts).toUInt();

    for (PortIndex portIndex = 0; portIndex < n; ++portIndex)
    {
      QPointF p = geom.portNodePosition(portType, portIndex);

      auto const &dataCaption =
        model.portData(nodeId,
                       portType,
                       portIndex,
                       PortRole::Caption).toString();

      double r = 1.0;

      NodeState const &state = ngo.nodeState();

      if (auto const * cgo = state.connectionForReaction())
      {
        PortType requiredPort =
          cgo->connectionState().requiredPort();

        if (requiredPort == portType)
        {

          ConnectionId possibleConnectionId =
            makeCompleteConnectionId(cgo->connectionId(),
                                     nodeId,
                                     portIndex);

          bool const possible = model.connectionPossible(possibleConnectionId);

          auto cp = cgo->sceneTransform().map(cgo->endPoint(requiredPort));
          cp = ngo.sceneTransform().inverted().map(cp);

          auto diff   = cp - p;
          double dist = std::sqrt(QPointF::dotProduct(diff, diff));

          if (possible)
          {
            double const thres = 40.0;
            r = (dist < thres) ?
                (2.0 - dist / thres ) :
                1.0;
          }
          else
          {
            double const thres = 80.0;
            r = (dist < thres) ?
                (dist / thres) :
                1.0;
          }
        }
      }

      QJsonDocument json =
        QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
      NodeStyle nodeStyle(json);
      const auto portColorType = model.portData(nodeId, portType, portIndex, PortRole::ColorType).toInt();
      const auto it = nodeStyle.ConnectionPointColorMap.find(portColorType);
      auto color = nodeStyle.ConnectionPointColor;
      if (it != nodeStyle.ConnectionPointColorMap.end())
        painter->setPen(*it);
      else
        painter->setPen(color);
      painter->setBrush(color);

      if (connectionStyle.useDataDefinedColors())
      {
        painter->setBrush(connectionStyle.normalColor(dataCaption));
      }
      else
      {
        painter->setBrush(color);
      }

      painter->drawEllipse(p,
                           reducedDiameter * r,
                           reducedDiameter * r);
    }
  }

  if (ngo.nodeState().connectionForReaction())
  {
    ngo.nodeState().resetConnectionForReaction();
  }
}


void
NodePainter::
drawFilledConnectionPoints(QPainter * painter,
                           NodeGraphicsObject &ngo)
{
  AbstractGraphModel const &model = ngo.graphModel();
  NodeId const nodeId     = ngo.nodeId();
  NodeGeometry geom(ngo);

  QJsonDocument json =
    QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
  NodeStyle nodeStyle(json);

  auto diameter = nodeStyle.ConnectionPointDiameter;

  for (PortType portType: {PortType::Out, PortType::In})
  {
    size_t const n =
      model.nodeData(nodeId,
                     (portType == PortType::Out) ?
                     NodeRole::NumberOfOutPorts :
                     NodeRole::NumberOfInPorts).toUInt();

    for (PortIndex portIndex = 0; portIndex < n; ++portIndex)
    {
      QPointF p = geom.portNodePosition(portType, portIndex);

      auto const &connectedNodes =
        model.connectedNodes(nodeId, portType, portIndex);

      if (!connectedNodes.empty())
      {
        auto const &colorType =
          model.portData(nodeId,
                         portType,
                         portIndex,
                         PortRole::ColorType).toString();

        auto const &connectionStyle = StyleCollection::connectionStyle();
        if (connectionStyle.useDataDefinedColors())
        {
          QColor const c = connectionStyle.normalColor(colorType);
          painter->setPen(c);
          painter->setBrush(c);
        }
        else
        {
          QColor color = nodeStyle.FilledConnectionPointColor;
          const auto& portColorType = model.portData(nodeId, portType, portIndex, PortRole::ColorType).toInt();
          const auto it = nodeStyle.ConnectionPointColorMap.find(portColorType);
          if (it != nodeStyle.ConnectionPointColorMap.end())
              color = *it;
          painter->setPen(color);
          painter->setBrush(color);
        }
        painter->drawEllipse(p,
                             diameter * 0.4,
                             diameter * 0.4);
      }

    }
  }
}


void
NodePainter::
drawNodeCaptionBackground(QPainter* painter,
    NodeGraphicsObject& ngo)
{
  AbstractGraphModel const& model = ngo.graphModel();

  NodeId const nodeId = ngo.nodeId();

  NodeGeometry geom(ngo);
  QSize size = geom.size();

  QJsonDocument json =
    QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));

  NodeStyle nodeStyle(json);

  auto color = ngo.isSelected() ?
    nodeStyle.SelectedBoundaryColor :
    nodeStyle.NormalBoundaryColor;

  if (ngo.nodeState().hovered())
  {
    QPen p(color, nodeStyle.HoveredPenWidth);
    painter->setPen(p);
  }
  else
  {
    QPen p(color, nodeStyle.PenWidth);
    painter->setPen(p);
  }

  const auto height = (geom.verticalSpacing() + geom.entryHeight()) / 3.0 + geom.entryHeight();

  QLinearGradient gradient(QPointF(0.0, 0.0), QPointF(2.0, height));

  gradient.setColorAt(0.0, nodeStyle.TitleGradientColor0);
  gradient.setColorAt(1.0, nodeStyle.TitleGradientColor1);
  painter->setBrush(gradient);

  float diam = nodeStyle.ConnectionPointDiameter;

  QRectF boundary(-diam, -diam,
                  2.0 * diam + size.width(),
                  height);

  double const radius = 3.0;

  painter->drawRoundedRect(boundary, radius, radius);
}


void
NodePainter::
drawNodeCaption(QPainter * painter,
                NodeGraphicsObject &ngo)
{
  AbstractGraphModel const &model = ngo.graphModel();
  NodeId const nodeId     = ngo.nodeId();
  NodeGeometry geom(ngo);

  if (!model.nodeData(nodeId, NodeRole::CaptionVisible).toBool())
    return;

  QString const name = model.nodeData(nodeId, NodeRole::Caption).toString();

  QFont f = painter->font();
  f.setBold(true);

  QFontMetrics metrics(f);
  auto rect  = metrics.boundingRect(name);
  QSize size = geom.size();

  QPointF position((size.width() - rect.width()) / 2.0,
                   (geom.verticalSpacing() + geom.entryHeight()) / 3.0);

  QJsonDocument json =
    QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
  NodeStyle nodeStyle(json);

  painter->setFont(f);
  painter->setPen(nodeStyle.FontColor);
  painter->drawText(position, name);

  f.setBold(false);
  painter->setFont(f);
}


void
NodePainter::
drawEntryLabels(QPainter * painter,
                NodeGraphicsObject &ngo)
{
  AbstractGraphModel const &model = ngo.graphModel();
  NodeId const nodeId     = ngo.nodeId();
  NodeGeometry geom(ngo);

  QJsonDocument json =
    QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
  NodeStyle nodeStyle(json);

  QSize size = geom.size();

  for (PortType portType: {PortType::Out, PortType::In})
  {
    size_t const n =
      model.nodeData(nodeId,
                     (portType == PortType::Out) ?
                     NodeRole::NumberOfOutPorts :
                     NodeRole::NumberOfInPorts).toUInt();

    for (PortIndex portIndex = 0; portIndex < n; ++portIndex)
    {
      auto const &connectedNodes =
        model.connectedNodes(nodeId, portType, portIndex);

      QPointF p = geom.portNodePosition(portType, portIndex);

      if (connectedNodes.empty())
        painter->setPen(nodeStyle.FontColorFaded);
      else
        painter->setPen(nodeStyle.FontColor);

      QString s;

      if (model.portData(nodeId, portType, portIndex, PortRole::CaptionVisible).toBool())
      {
        s = model.portData(nodeId, portType, portIndex, PortRole::Caption).toString();
      }
      else
      {
        auto portData =
          model.portData(nodeId, portType, portIndex, PortRole::DataType);

        s = portData.value<NodeDataType>().name;
      }

      QFontMetrics const &metrics = painter->fontMetrics();
      auto rect = metrics.boundingRect(s);

      p.setY(p.y() + rect.height() / 4.0);

      switch (portType)
      {
        case PortType::In:
          p.setX(5.0);
          break;

        case PortType::Out:
          p.setX(size.width() - 5.0 - rect.width());
          break;

        default:
          break;
      }

      painter->drawText(p, s);
    }
  }
}


void
NodePainter::
drawResizeRect(QPainter * painter,
               NodeGraphicsObject &ngo)
{
  AbstractGraphModel const &model = ngo.graphModel();
  NodeId const nodeId     = ngo.nodeId();
  NodeGeometry geom(ngo);

  if (model.nodeFlags(nodeId) & NodeFlag::Resizable)
  {
    painter->setBrush(Qt::gray);

    painter->drawEllipse(geom.resizeRect());
  }
}


}
