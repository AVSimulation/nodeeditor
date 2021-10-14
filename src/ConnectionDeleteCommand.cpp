#include "ConnectionDeleteCommand.hpp"



namespace QtNodes
{
    //------------------------------------------------------------------------------------------
    ConnectionDeleteCommand::ConnectionDeleteCommand(QUndoCommand* parent) :
        QUndoCommand(parent)
    {
    }

    //------------------------------------------------------------------------------------------
    ConnectionDeleteCommand::~ConnectionDeleteCommand()
    {
    }
};
