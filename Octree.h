///////////////////////////////////////////////////////////////////////////////////
//  Octree c++ implementation
//
//  Copyright (c) [2015] [Adrian Krupa]
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
///////////////////////////////////////////////////////////////////////////////////

#ifndef __AKOctree__Octree__
#define __AKOctree__Octree__

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stack>
#include <queue>

namespace AKOctree {

#pragma mark Forward declarations

    template<class LeafDataType, class NodeDataType>
    class Octree;

    template<class LeafDataType, class NodeDataType>
    class OctreeCell;

    template<class LeafDataType, class NodeDataType>
    class OctreeLeaf;

    template<class LeafDataType, class NodeDataType>
    class OctreeVisitor;

#pragma mark Axis visualization

    /*
       y
       |
       +--x
      /
     z
         0----1
        /|   /|
       2-+--3 |
       | 4--+-5
       |/   |/
       6----7
     
       000---001
       /|    /|
      / |   / |
    010-+-011 |
     | 100-+-101
     | /   | /
     |/    |/
    110---111
     */

#pragma mark OctreeAgent base class

    template<class LeafDataType, class NodeDataType = LeafDataType>
    class OctreeAgent {

    public:
        virtual ~OctreeAgent() {}

        virtual bool isItemOverlappingCell(const LeafDataType *item,
                                           const glm::vec3 &cellCenter,
                                           const float &cellRadius) const = 0;

        virtual glm::vec3 GetMaxValuesForAutoAdjust(const LeafDataType *item,
                                                    const glm::vec3 &max) const;

        virtual glm::vec3 GetMinValuesForAutoAdjust(const LeafDataType *item,
                                                    const glm::vec3 &min) const;

    protected:
        OctreeAgent() {}
    };

    template<class LeafDataType, class NodeDataType = LeafDataType>
    class OctreeCellInterface {
        virtual NodeDataType &GetBranchData() const = 0;

        virtual const glm::vec3 GetCellCenter() const = 0;

        virtual const float GetCellRadius() const = 0;
    };

    template<class LeafDataType, class NodeDataType = LeafDataType>
    class OctreeCell : OctreeCellInterface<LeafDataType, NodeDataType> {

        typedef Octree<LeafDataType, NodeDataType> OctreeLN;
        typedef OctreeCell<LeafDataType, NodeDataType> OctreeCellLN;
        typedef OctreeAgent<LeafDataType, NodeDataType> OctreeAgentLN;
        typedef OctreeVisitor<LeafDataType, NodeDataType> OctreeVisitorLN;

    public:
        virtual void insert(OctreeCellLN *&pThis, const LeafDataType *item, const OctreeAgentLN &agent) = 0;

        virtual bool insertInThread(OctreeCellLN *&pThis, const LeafDataType *item, const OctreeAgentLN &agent) = 0;

        virtual void PrintTreeAndSubtree(unsigned int level) const = 0;

        virtual bool GetItemPath(LeafDataType *item, std::string &path) const = 0;

        virtual void visit(const OctreeVisitorLN &visitor) const = 0;

        virtual unsigned int ForceCountItems() const = 0;

        virtual bool CanMoveCell() const = 0;

        virtual ~OctreeCell() {}

        virtual bool isLeaf() const;

        virtual NodeDataType &GetBranchData() const override;

        void MoveCell(glm::vec3 center, float radius);

        virtual const glm::vec3 GetCellCenter() const override;

        virtual const float GetCellRadius() const override;

        friend bool operator==(const OctreeCellLN &lhs, const OctreeCellLN &rhs) {
            return lhs.equal_to(rhs);
        }

        friend bool operator!=(OctreeCellLN const &lhs, OctreeCellLN const &rhs) {
            return !(lhs == rhs);
        }

    protected:
        OctreeCell(OctreeLN *octree,
                   glm::vec3 center,
                   float radius) : baseOctree(octree),
                                   center(center),
                                   radius(radius) {}

        virtual bool equal_to(OctreeCellLN const &other) const = 0;

        OctreeLN *baseOctree;

        glm::vec3 center = glm::vec3(0);
        float radius = 0.0f;
        mutable NodeDataType branchData;
    };

    template<class LeafDataType, class NodeDataType = LeafDataType>
    class OctreeBranch : public OctreeCell<LeafDataType, NodeDataType> {

        typedef Octree<LeafDataType, NodeDataType> OctreeLN;
        typedef OctreeCell<LeafDataType, NodeDataType> OctreeCellLN;
        typedef OctreeBranch<LeafDataType, NodeDataType> OctreeBranchLN;
        typedef OctreeLeaf<LeafDataType, NodeDataType> OctreeLeafLN;
        typedef OctreeAgent<LeafDataType, NodeDataType> OctreeAgentLN;
        typedef OctreeVisitor<LeafDataType, NodeDataType> OctreeVisitorLN;

    public:
        OctreeBranch(OctreeLN *octree,
                     glm::vec3 center,
                     float radius,
                     const std::vector<const LeafDataType *> &items,
                     const LeafDataType *item,
                     const OctreeAgentLN &agent);

        virtual void insert(OctreeCellLN *&pThis, const LeafDataType *item, const OctreeAgentLN &agent) override;

        const OctreeCellLN *const *GetChilds() const;

        virtual bool insertInThread(OctreeCellLN *&pThis, const LeafDataType *item, const OctreeAgentLN &agent) override;

        virtual unsigned int ForceCountItems() const override;

        virtual bool CanMoveCell() const override;

        virtual void PrintTreeAndSubtree(unsigned int level) const override;

        virtual bool GetItemPath(LeafDataType *item, std::string &path) const override;

        virtual void visit(const OctreeVisitor<LeafDataType, NodeDataType> &visitor) const override;

    protected:
        virtual bool equal_to(OctreeCell<LeafDataType, NodeDataType> const &other) const override;

    private:
        OctreeCell<LeafDataType, NodeDataType> *childs[8];
    };

    template<class LeafDataType, class NodeDataType>
    class OctreeLeaf : public OctreeCell<LeafDataType, NodeDataType> {

        typedef Octree<LeafDataType, NodeDataType> OctreeLN;
        typedef OctreeCell<LeafDataType, NodeDataType> OctreeCellLN;
        typedef OctreeBranch<LeafDataType, NodeDataType> OctreeBranchLN;
        typedef OctreeLeaf<LeafDataType, NodeDataType> OctreeLeafLN;
        typedef OctreeAgent<LeafDataType, NodeDataType> OctreeAgentLN;
        typedef OctreeVisitor<LeafDataType, NodeDataType> OctreeVisitorLN;

        std::mutex leafMutex;

    public:

        OctreeLeaf(OctreeLN *octree, glm::vec3 center, float radius)
                : OctreeCellLN(octree, center, radius) {}

        virtual void insert(OctreeCellLN *&pThis, const LeafDataType *item, const OctreeAgentLN &agent) override;

        virtual bool insertInThread(OctreeCellLN *&pThis, const LeafDataType *item, const OctreeAgentLN &agent) override;

        virtual void PrintTreeAndSubtree(unsigned int level) const override;

        virtual bool GetItemPath(LeafDataType *item, std::string &path) const override;

        virtual bool CanMoveCell() const override;

        virtual bool isLeaf() const override;

        virtual unsigned int ForceCountItems() const override;

        virtual void visit(const OctreeVisitorLN &visitor) const override;

    protected:
        virtual bool equal_to(OctreeCellLN const &other) const override;

    private:
        std::vector<const LeafDataType *> data;
    };

    template<class LeafDataType, class NodeDataType = LeafDataType>
    class OctreeVisitor {
    public:
        virtual ~OctreeVisitor() {}

        virtual void visitRoot(const OctreeCell<LeafDataType, NodeDataType> *rootCell) const {
            ContinueVisit(rootCell);
        }

        virtual void visitPreRoot(const OctreeCell<LeafDataType, NodeDataType> *rootCell) const {
        }

        virtual void visitPostRoot(const OctreeCell<LeafDataType, NodeDataType> *rootCell) const {
        }

        virtual void visitPreBranch(const OctreeCell<LeafDataType, NodeDataType> *const childs[8], NodeDataType &branchData) const {
        }

        virtual void visitPostBranch(const OctreeCell<LeafDataType, NodeDataType> *const childs[8], NodeDataType &branchData) const {
        }

        virtual void visitBranch(const OctreeCell<LeafDataType, NodeDataType> *const childs[8], NodeDataType &branchData) const {
            printf("Visit branch\n");
            for (int i = 0; i < 8; ++i) {
                ContinueVisit(childs[i]);
            }
        }

        virtual void visitLeaf(const std::vector<const LeafDataType *> &items, NodeDataType &branchData) const {
            printf("Visit leaf\n");
        }

    protected:
        OctreeVisitor() {
        }

        void ContinueVisit(const OctreeCell<LeafDataType, NodeDataType> *cell) const {
            if (cell != nullptr) {
                cell->visit(*this);
            }
        }

    };

    template<class LeafDataType, class NodeDataType = LeafDataType>
    class Octree {

        glm::vec3 center = glm::vec3(0);
        float radius = 10.0f;

        unsigned int maxItemsPerCell = 1;
        unsigned int maxLevelCount = 0;
        float maxCellSize = 0.0f;

        unsigned int itemsCount = 0;
        OctreeCell<LeafDataType, NodeDataType> *root = nullptr;
        mutable unsigned int threadsNumber = 1;
        mutable std::mutex threadsMutex;

        std::vector<LeafDataType *> itemsToAdd;
        mutable std::stack<OctreeCell<LeafDataType, NodeDataType> *> cellsToProcess;
        mutable std::vector<std::thread> threads;


    public:
        Octree(unsigned int maxItemsPerCell, unsigned int maxLevelCount, float maxCellSize, unsigned int threadsNumber = 1)
                : maxItemsPerCell(maxItemsPerCell),
                  maxLevelCount(maxLevelCount),
                  maxCellSize(maxCellSize),
                  threadsNumber(threadsNumber) {
            root = new OctreeLeaf<LeafDataType, NodeDataType>(this, center, radius);
        }

        Octree(unsigned int maxItemsPerCell, unsigned int maxLevelCount, float maxCellSize, glm::vec3 center, float radius, unsigned int threadsNumber = 1)
                : maxItemsPerCell(maxItemsPerCell),
                  maxLevelCount(maxLevelCount),
                  maxCellSize(maxCellSize),
                  center(center),
                  radius(radius), threadsNumber(threadsNumber) {
            root = new OctreeLeaf<LeafDataType, NodeDataType>(this, center, radius);
        }

        ~Octree() {
        }

        void Clear() {
            delete root;
            root = new OctreeLeaf<LeafDataType, NodeDataType>(this, center, radius);
            itemsCount = 0;
        }

        void insert(const LeafDataType *item, const OctreeAgent<LeafDataType> &agent) {

            if (agent.isItemOverlappingCell(item, center, radius)) {
                root->insert(root, item, agent);
                itemsCount++;
            }
        }

        void insert(LeafDataType *items, const unsigned int itemsCount, const OctreeAgent<LeafDataType> &agent, bool autoAdjustTree = false) {

            if (autoAdjustTree) {
                glm::vec3 max = center + radius;
                glm::vec3 min = center - radius;
                for (int i = 0; i < itemsCount; ++i) {
                    max = agent.GetMaxValuesForAutoAdjust(&items[i], max);
                    min = agent.GetMinValuesForAutoAdjust(&items[i], min);
                }
                center = (max + min) / 2.0f;
                radius = glm::max(glm::abs(center.x - max.x), glm::abs(center.y - max.y));
                radius = glm::max(radius, glm::abs(center.z - max.z));
                radius *= 1.1f;
                //printf("New center: (%lf, %lf, %lf), new radius: %lf\n", center.x, center.y, center.z, radius);
                root->MoveCell(center, radius);
            }


            if (threadsNumber != 1) {
                itemsToAdd.reserve(itemsCount);
                for (int i = 0; i < itemsCount; ++i) {
                    itemsToAdd.push_back(&items[i]);
                }
                if (threadsNumber == 0) {
                    threadsNumber = std::thread::hardware_concurrency();
                }
                for (int i = 0; i < threadsNumber; ++i) {
                    threads.push_back(std::thread(&Octree::insertThread, this, std::ref(agent)));
                }
                for (int i = 0; i < threadsNumber; ++i) {
                    threads[i].join();
                }
                threads.clear();
            } else {
                for (int i = 0; i < itemsCount; ++i) {
                    if (agent.isItemOverlappingCell(&items[i], center, radius)) {
                        root->insert(root, &items[i], agent);
                        this->itemsCount++;
                    }
                }
            }
        }

        void insert(std::vector<LeafDataType> &items, const OctreeAgent<LeafDataType> &agent) {
            for (int i = 0; i < items.size(); ++i) {
                if (agent.isItemOverlappingCell(&items[i], center, radius)) {
                    root->insert(root, &items[i], agent);
                    this->itemsCount++;
                }
            }
        }

        void visit(OctreeVisitor<LeafDataType, NodeDataType> &visitor) const {
            if (threadsNumber != 1) {
                visitor.visitPreRoot(root);
                if (root->isLeaf()) {
                    root->visit(visitor);
                } else {
                    if (threadsNumber == 0) {
                        threadsNumber = std::thread::hardware_concurrency();
                    }
                    auto rootBranch = (OctreeBranch<LeafDataType, NodeDataType> *) root;
                    visitor.visitPreBranch(rootBranch->GetChilds(), root->GetBranchData());
                    int th[8];
                    for (int i = 0; i < threadsNumber; ++i) {
                        th[i] = i;
                        threads.push_back(std::thread(&Octree::visitThread, this, std::ref(visitor), std::ref(th[i])));
                    }
                    for (int i = 0; i < threadsNumber; ++i) {
                        threads[i].join();
                    }
                    threads.clear();
                    visitor.visitPostBranch(rootBranch->GetChilds(), root->GetBranchData());
                }

                visitor.visitPostRoot(root);
            } else {
                visitor.visitRoot(root);
            }
        }

        unsigned int GetMaxItemsPerCell() const {
            return maxItemsPerCell;
        }

        unsigned int GetMaxLevelCount() const {
            return maxLevelCount;
        }

        float GetMaxCellSize() const {
            return maxCellSize;
        }

        void PrintTree() const {
            root->PrintTreeAndSubtree(0);
        }

        unsigned int GetItemsCount() const {
            return itemsCount;
        }

        unsigned int ForceGetItemsCount() const {
            return root->ForceCountItems();
        }

        std::string GetItemPath(LeafDataType *item) {
            std::string v;

            root->GetItemPath(item, v);

            return v;
        }

        bool operator==(const Octree<LeafDataType, NodeDataType> &rhs) {
            return *root == *rhs.root;
        }

    private:
        void insertThread(const OctreeAgent<LeafDataType> &agent) {
            unsigned int itemsToBatch = std::max(threadsNumber, 1u) * 2;
            while (!itemsToAdd.empty()) {
                threadsMutex.lock();
                if (itemsToAdd.empty()) {
                    threadsMutex.unlock();
                    break;
                }
                std::vector<LeafDataType *> tempItems;
                tempItems.reserve(itemsToBatch);
                for (int i = 0; i < itemsToBatch; ++i) {
                    if (itemsToAdd.empty()) {
                        break;
                    }
                    LeafDataType *item = itemsToAdd[itemsToAdd.size() - 1];
                    tempItems.push_back(item);
                    itemsToAdd.pop_back();
                    this->itemsCount++;
                }
                threadsMutex.unlock();
                for (int i = 0; i < tempItems.size(); ++i) {
                    LeafDataType *item = tempItems[i];
                    if (agent.isItemOverlappingCell(item, center, radius)) {
                        while (!root->insertInThread(root, item, agent));
                    }
                }

            }
        }

        void visitThread(const OctreeVisitor<LeafDataType> &visitor, int thread) const {

            if (root->isLeaf()) {
                return;
            }
            auto rootBranch = (OctreeBranch<LeafDataType, NodeDataType> *) root;

            if (threadsNumber <= 8) {
                auto childs = rootBranch->GetChilds();
                float nodesPerThread = 8.0f / threadsNumber;
                for (int i = (int) (thread * nodesPerThread); i < (int) ((thread + 1) * nodesPerThread); ++i) {
                    childs[i]->visit(visitor);
                }
            }
        }

    };

#pragma mark OctreeAgent implementation

    template<class LeafDataType, class NodeDataType>
    glm::vec3 OctreeAgent<LeafDataType, NodeDataType>::GetMaxValuesForAutoAdjust(const LeafDataType *item, const glm::vec3 &max) const {
        printf("Override OctreeAgent::GetMaxValuesForAutoAdjust if you wan't to use auto adjust!\n");
        return max;
    }

    template<class LeafDataType, class NodeDataType>
    glm::vec3 OctreeAgent<LeafDataType, NodeDataType>::GetMinValuesForAutoAdjust(const LeafDataType *item, const glm::vec3 &min) const {
        printf("Override OctreeAgent::GetMinValuesForAutoAdjust if you wan't to use auto adjust!\n");
        return min;
    }

#pragma mark OctreeCell implementation

    template<class LeafDataType, class NodeDataType>
    bool OctreeCell<LeafDataType, NodeDataType>::isLeaf() const {
        return false;
    }

    template<class LeafDataType, class NodeDataType>
    NodeDataType &OctreeCell<LeafDataType, NodeDataType>::GetBranchData() const {
        return branchData;
    }

    template<class LeafDataType, class NodeDataType>
    void OctreeCell<LeafDataType, NodeDataType>::MoveCell(glm::vec3 center, float radius) {
        this->center = center;
        this->radius = radius;
    }

    template<class LeafDataType, class NodeDataType>
    const glm::vec3 OctreeCell<LeafDataType, NodeDataType>::GetCellCenter() const {
        return center;
    }

    template<class LeafDataType, class NodeDataType>
    const float OctreeCell<LeafDataType, NodeDataType>::GetCellRadius() const {
        return radius;
    }

#pragma mark OctreeBranch implementation

    template<class LeafDataType, class NodeDataType>
    OctreeBranch<LeafDataType, NodeDataType>::OctreeBranch(OctreeLN *octree,
                                                           glm::vec3 center,
                                                           float radius,
                                                           const std::vector<const LeafDataType *> &items,
                                                           const LeafDataType *item,
                                                           const OctreeAgentLN &agent)
            : OctreeCellLN(octree, center, radius) {

        OctreeCellLN *notUsedPointer = nullptr;
        float halfRadius = radius / 2.0f;
        for (int i = 0; i < 8; ++i) {
            bool up = i < 4;//+y
            bool right = (i & 1) == 1;//+x
            bool front = ((i >> 1) & 1) != 0; //+z
            glm::vec3 newCenter = this->center + glm::vec3(right ? halfRadius : -halfRadius,
                                                           up ? halfRadius : -halfRadius,
                                                           front ? halfRadius : -halfRadius);
            childs[i] = new OctreeLeafLN(this->baseOctree, newCenter, halfRadius);
        }

        for (int i = 0; i < items.size(); ++i) {
            insert(notUsedPointer, items[i], agent);
        }
        insert(notUsedPointer, item, agent);
    }

    template<class LeafDataType, class NodeDataType>
    void OctreeBranch<LeafDataType, NodeDataType>::insert(OctreeCellLN *&pThis, const LeafDataType *item, const OctreeAgentLN &agent) {

        float halfRadius = this->radius / 2.0f;
        for (int i = 0; i < 8; ++i) {
            bool up = i < 4;//+y
            bool right = (i & 1) == 1;//+x
            bool front = ((i >> 1) & 1) != 0; //+z
            glm::vec3 newCenter = this->center + glm::vec3(right ? halfRadius : -halfRadius,
                                                           up ? halfRadius : -halfRadius,
                                                           front ? halfRadius : -halfRadius);
            if (agent.isItemOverlappingCell(item, newCenter, halfRadius)) {
                childs[i]->insert(childs[i], item, agent);
                break;
            }
        }
    }

    template<class LeafDataType, class NodeDataType>
    const OctreeCell<LeafDataType, NodeDataType> *const *OctreeBranch<LeafDataType, NodeDataType>::GetChilds() const {
        return childs;
    }

    template<class LeafDataType, class NodeDataType>
    bool OctreeBranch<LeafDataType, NodeDataType>::insertInThread(OctreeCellLN *&pThis, const LeafDataType *item, const OctreeAgentLN &agent) {
        float halfRadius = this->radius / 2.0f;
        for (int i = 0; i < 8; ++i) {
            bool up = i < 4;//+y
            bool right = (i & 1) == 1;//+x
            bool front = ((i >> 1) & 1) != 0; //+z
            glm::vec3 newCenter = this->center + glm::vec3(right ? halfRadius : -halfRadius,
                                                           up ? halfRadius : -halfRadius,
                                                           front ? halfRadius : -halfRadius);
            if (agent.isItemOverlappingCell(item, newCenter, halfRadius)) {
                while (!childs[i]->insertInThread(childs[i], item, agent));
                break;
            }
        }
        return true;
    }

    template<class LeafDataType, class NodeDataType>
    unsigned int OctreeBranch<LeafDataType, NodeDataType>::ForceCountItems() const {
        unsigned int count = 0;
        for (int i = 0; i < 8; ++i) {
            count += childs[i]->ForceCountItems();
        }
        return count;
    }

    template<class LeafDataType, class NodeDataType>
    bool OctreeBranch<LeafDataType, NodeDataType>::CanMoveCell() const {
        bool can = true;
        for (int i = 0; i < 8; ++i) {
            if (!childs[i]->CanMoveCell()) {
                can = false;
                break;
            }
        }
        return can;
    }

    template<class LeafDataType, class NodeDataType>
    void OctreeBranch<LeafDataType, NodeDataType>::PrintTreeAndSubtree(unsigned int level) const {
        printf("Branch ");
        for (int i = 0; i < 8; ++i) {
            if (i != 0) {
                for (int j = 0; j < level + 1; ++j) {
                    printf("       ");
                }
                for (int j = 0; j < level; ++j) {
                    printf("  ");
                }
            }
            printf("%d ", i);
            if (childs[i] == nullptr) {
                printf("NULL\n");
            } else {
                childs[i]->PrintTreeAndSubtree(level + 1);
            }

        }
    }

    template<class LeafDataType, class NodeDataType>
    bool OctreeBranch<LeafDataType, NodeDataType>::GetItemPath(LeafDataType *item, std::string &path) const {

        for (int i = 0; i < 8; ++i) {
            path += std::to_string(i);
            if (childs[i]->GetItemPath(item, path)) {
                return true;
            } else {
                path.pop_back();
            }
        }
        return false;
    }

    template<class LeafDataType, class NodeDataType>
    void OctreeBranch<LeafDataType, NodeDataType>::visit(const OctreeVisitorLN &visitor) const {
        visitor.visitBranch(childs, this->branchData);
    }

    template<class LeafDataType, class NodeDataType>
    bool OctreeBranch<LeafDataType, NodeDataType>::equal_to(OctreeCellLN const &other) const {
        if (OctreeBranchLN const *p = dynamic_cast<OctreeBranchLN const *>(&other)) {
            for (int i = 0; i < 8; ++i) {
                bool equal = *childs[i] == *p->childs[i];
                if (!equal) {
                    return false;
                }
            }
        }
        return true;
    }

#pragma mark OctreeLeaf implementation

    template<class LeafDataType, class NodeDataType>
    void OctreeLeaf<LeafDataType, NodeDataType>::insert(OctreeCellLN *&pThis, const LeafDataType *item, const OctreeAgentLN &agent) {

        for (int i = 0; i < data.size(); ++i) {
            if (data[i] == item) {
                return;
            }
        }
        if (this->baseOctree->GetMaxItemsPerCell() <= data.size()) {
            OctreeCellLN *cell = new OctreeBranchLN(this->baseOctree, this->center, this->radius, data, item, agent);
            delete pThis;
            pThis = cell;
        } else {
            data.push_back(item);
        }
    }

    template<class LeafDataType, class NodeDataType>
    bool OctreeLeaf<LeafDataType, NodeDataType>::insertInThread(OctreeCellLN *&pThis, const LeafDataType *item, const OctreeAgentLN &agent) {
        if (!leafMutex.try_lock()) {
            return false;
        }
        if (this->baseOctree->GetMaxItemsPerCell() <= data.size()) {
            OctreeCellLN *cell = new OctreeBranchLN(this->baseOctree, this->center, this->radius, data, item, agent);
            delete pThis;
            pThis = cell;
        } else {
            data.push_back(item);
            leafMutex.unlock();
        }
        return true;
    }

    template<class LeafDataType, class NodeDataType>
    void OctreeLeaf<LeafDataType, NodeDataType>::PrintTreeAndSubtree(unsigned int level) const {
        printf("Leaf, items:%lu ", data.size());
        for (unsigned int i = 0; i < data.size(); ++i) {
            printf("%llu ", (unsigned long long) data[i]);
        }
        printf("\n");
    }

    template<class LeafDataType, class NodeDataType>
    bool OctreeLeaf<LeafDataType, NodeDataType>::GetItemPath(LeafDataType *item, std::string &path) const {

        for (auto &it : data) {
            if (it == item) {
                return true;
            }
        }
        return false;
    }

    template<class LeafDataType, class NodeDataType>
    bool OctreeLeaf<LeafDataType, NodeDataType>::CanMoveCell() const {
        return data.empty();
    }

    template<class LeafDataType, class NodeDataType>
    bool OctreeLeaf<LeafDataType, NodeDataType>::isLeaf() const {
        return true;
    }

    template<class LeafDataType, class NodeDataType>
    unsigned int OctreeLeaf<LeafDataType, NodeDataType>::ForceCountItems() const {
        return (unsigned int) data.size();
    }

    template<class LeafDataType, class NodeDataType>
    void OctreeLeaf<LeafDataType, NodeDataType>::visit(const OctreeVisitorLN &visitor) const {
        visitor.visitLeaf(data, this->branchData);
    }

    template<class LeafDataType, class NodeDataType>
    bool OctreeLeaf<LeafDataType, NodeDataType>::equal_to(OctreeCellLN const &other) const {
        if (OctreeLeafLN const *p = dynamic_cast<OctreeLeafLN const *>(&other)) {
            if (data.size() != p->data.size()) {
                return false;
            }
            for (int i = 0; i < data.size(); ++i) {
                bool found = false;
                for (int j = 0; j < data.size(); ++j) {
                    if (data[i] == p->data[j]) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return false;
                }
            }
        }
        return true;
    }
}

#endif /* defined(__AKOctree__Octree__) */
