#include "amdornode.h"

AmdorNode::AmdorNode(int id, AmdorNode *_parent) : freqIndex(id), parent(_parent)
{
    parentIndex = 0;
}

AmdorNode::~AmdorNode()
{
    while(!children.isEmpty())
        delete children.takeFirst();
}

AmdorNode *AmdorNode::addChild(int id)
{
    AmdorNode *child = new AmdorNode(id,this);
    children.append(child);
    child->parentIndex = children.indexOf(child);
    return child;
}

bool AmdorNode::addChildToTree(int ftId, int drId)
{
    if(!isRootNode())
        return parent->addChildToTree(ftId, drId);

    for(int i=0; i<children.size(); i++)
    {
        if(children.at(i)->_tryAddChildInternal(ftId,drId))
            return true;
    }

    return false;

}

bool AmdorNode::_tryAddChildInternal(int ftId, int drId)
{
    //this function is meant to be used internally from a non-root node
    if(isRootNode())
        return addChildToTree(ftId,drId);

    if(parent->freqIndex == ftId) //this might be a child
    {
        //make sure this drId is not already a sibling!
        for(int i=0; i<parent->children.size(); i++)
        {
            if(parent->children.at(i)->parentIndex == i)
                continue;

            if(parent->children.at(i)->freqIndex == drId)
                return false;
        }

        //at this point, this is a confirmed new linkage
        addChild(drId);
        return true;
    }

    //check children
    for(int i=0; i<children.size(); i++)
    {
        if(children.at(i)->_tryAddChildInternal(ftId,drId))
            return true;
    }

    return false;
}

bool AmdorNode::isRootNode()
{
    return parent == nullptr;
}

AmdorNode *AmdorNode::rootNode()
{
    if(isRootNode())
        return this;
    else
        return parent->rootNode();
}

int AmdorNode::totalTreeSize()
{
    return rootNode()->treeSize();
}

int AmdorNode::treeSize()
{
    int out = 1;
    for(int i=0; i<children.size(); i++)
        out += children.at(i)->treeSize();

    return out;
}
