#ifndef AMDORNODE_H
#define AMDORNODE_H

#include <QList>

class AmdorNode
{
public:
    AmdorNode(int id, AmdorNode *_parent = nullptr);
    ~AmdorNode();

    int freqIndex;
    int parentIndex;
    AmdorNode *parent;
    QList<AmdorNode*> children;

    AmdorNode *addChild(int id);

    //attempts to add a node to the tree.
    //return whether or not it was added
    bool addChildToTree(int ftId, int drId);
    bool _tryAddChildInternal(int ftId, int drId);

    bool isRootNode();
    AmdorNode *rootNode();
    int totalTreeSize();
    int treeSize();
};

#endif // AMDORNODE_H
