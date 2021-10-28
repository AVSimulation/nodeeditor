#pragma once

#include <QtWidgets/QGraphicsView>
#include "Definitions.hpp"
#include "Export.hpp"

#include "ConnectionGraphicsObject.hpp"
#include "NodeGraphicsObject.hpp"
#include "BasicGraphicsScene.hpp"
#include "StyleCollection.hpp"


namespace QtNodes
{
	class BasicGraphicsScene;


	class NODE_EDITOR_PUBLIC GraphicsView : public QGraphicsView
	{
		Q_OBJECT

	public:
		GraphicsView(QWidget *parent = Q_NULLPTR);
		GraphicsView(BasicGraphicsScene *scene, QWidget *parent = Q_NULLPTR);
		GraphicsView(const GraphicsView &) = delete;
		GraphicsView operator=(const GraphicsView &) = delete;

		QAction* clearSelectionAction() const;
		QAction* deleteSelectionAction() const;
		void setScene(BasicGraphicsScene *scene);
		void centerScene();

	public Q_SLOTS:
		void scaleUp();
		void scaleDown();
		void deleteSelectedObjects();

	Q_SIGNALS:
		void requestDeleteConnections(const std::vector<ConnectionId>& cnxList);
		void requestDeleteNodes(const std::vector<NodeGraphicsObject*>& nodeList);
		void requestDeleteObjects(const std::vector<ConnectionId>& cnxList, const std::vector<NodeGraphicsObject*>& nodeList);

	protected:
		void contextMenuEvent(QContextMenuEvent *event) override;
		void wheelEvent(QWheelEvent *event) override;
		void keyPressEvent(QKeyEvent *event) override;
		void keyReleaseEvent(QKeyEvent *event) override;
		void mousePressEvent(QMouseEvent *event) override;
		void mouseMoveEvent(QMouseEvent *event) override;
		void drawBackground(QPainter* painter, const QRectF & r) override;
		void showEvent(QShowEvent *event) override;

	protected:
		BasicGraphicsScene*  nodeScene();

	private:
		QAction* _clearSelectionAction;
		QAction* _deleteSelectionAction;
		QPointF _clickPos;
	};
}
