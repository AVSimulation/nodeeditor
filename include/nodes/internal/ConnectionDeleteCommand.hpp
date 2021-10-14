#pragma once

#include <QUndoStack>
#include <QUndoCommand>
#include <QtWidgets/QGraphicsScene>

#include "Export.hpp"

namespace QtNodes
{
	class NODE_EDITOR_PUBLIC ConnectionDeleteCommand : public QUndoCommand
	{

	public:
		ConnectionDeleteCommand(QUndoCommand* parent = nullptr);
		~ConnectionDeleteCommand();

		//void undo() override;
		//void redo() override;

	private:
		QGraphicsScene* myGraphicsScene;
		QPointF initialPosition;
	};
};
