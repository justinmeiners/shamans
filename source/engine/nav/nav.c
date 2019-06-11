
#include "nav.h"
#include <stdlib.h>
#include "stretchy_buffer.h"
#include "platform.h"
#include <assert.h>

/*
 
 AI Path Finding round 10
 
 It seems like every time I start writing a game prototype I eventually get require pathfinding. 
 This time I am documenting my research so I don't have to research again.
 
 - Path Finding finds an ideal path for a game object to get from one point to another in a world.
 - It is the ideal, not actual path.  Game objects follow paths and use collision avoidance and to avoid dynamic obstacles to the best of their abilities.
 - Path finding does not solve world collision detection
 - A* (star) is the industry standard algorithm for calculating paths.
 - A* works by traversing a set of nodes, with a cost based linkage between them
 - Nodes can be represented as grid cells, waypoints, or specific points on a meshi
 - no matter how good the path is at the time it is calculated, things can change over time.
 - http://www.policyalmanac.org/games/aStarTutorial.htm
 - http://digestingduck.blogspot.com/
 */

static float TriArea(const Vec3* a, const Vec3* b, const Vec3* c)
{
    float ax = b->x - a->x;
    float ay = b->y - a->y;
    float bx = c->x - a->x;
    float by = c->y - a->y;
    return bx * ay - ax * by;
}

static int VecEqual(const Vec3* a, const Vec3* b)
{
    static const float eq = 0.001f*0.001f;
    return ((a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y)) < eq;
}

void NavPath_Init(NavPath* path)
{
    path->nodeCount = 0;
    memset(path->nodes, 0x0, sizeof(path->nodes));
}

void NavPath_AddNode(NavPath* path, Vec3 position, int edgeIndex, int polyIndex)
{
    assert(path->nodeCount + 1 < PATH_MAX_NODES);
    
    NavPathNode newNode;
    newNode.point = position;
    newNode.polyIndex = polyIndex;
    newNode.edgeIndex = edgeIndex;
    
    path->nodes[path->nodeCount] = newNode;
    ++path->nodeCount;
}

NavPathNode* NavPath_NodeAtIndex(NavPath* path, int index)
{
    assert(index < path->nodeCount);
    return &path->nodes[index];
}

void NavPath_Clear(NavPath* path)
{
    assert(path);
    path->nodeCount = 0;
}

void NavPath_Reverse(const NavPath* inPath, NavPath* outPath)
{
    if (!inPath || !outPath) return;

    outPath->nodeCount = inPath->nodeCount;
    
    for (int i = 0; i < inPath->nodeCount; i ++)
        outPath->nodes[inPath->nodeCount - (i + 1)] = inPath->nodes[i];
}

Vec3 NavPath_PointAt(NavPath* path, float distance)
{
    if (path->nodeCount < 1) return Vec3_Zero;
    
    Vec3 previous = path->nodes[0].point;
    
    for (int i = 1; i < path->nodeCount; ++i)
    {
        Vec3 current = path->nodes[i].point;
        
        float distanceToPrevious = Vec3_Dist(current, previous);
        
        if (distance > distanceToPrevious)
        {
            distance -= distanceToPrevious;
        }
        else
        {
            return Vec3_Lerp(previous, current, distance / distanceToPrevious);
        }
        
        previous = current;
    }
    
    return path->nodes[path->nodeCount - 1].point;
}


int NavSolver_Init(NavSolver* nav)
{
    nav->pool = NULL;
    nav->closed = NULL;
    
    return 1;
}

void NavSolver_Shutdown(NavSolver* nav)
{
    if (nav->pool)
        stb_sb_free(nav->pool);
    
    if (nav->closed)
        stb_sb_free(nav->closed);
    
    nav->pool = NULL;
    nav->closed = NULL;
}

void NavSolver_Prepare(NavSolver* nav,
                       const NavMesh* mesh)
{
    stb_sb_free(nav->pool);
    stb_sb_free(nav->closed);
    
    nav->pool = NULL;
    nav->closed = NULL;
    
    stb_sb_add(nav->pool, mesh->polyCount);
    stb_sb_add(nav->closed, mesh->polyCount);
}


/* A* path finding */
int NavSolver_Solve(NavSolver* nav,
                    const NavMesh* mesh,
                    Vec3 startPoint,
                    Vec3 endPoint,
                    const NavPoly* startPoly,
                    const NavPoly* endPoly,
                    NavPath* outPath)
{
    if (!nav || !mesh || !startPoly || !endPoly || mesh->polyCount < 1)
        return 0;
    
    NavPath_Clear(outPath);
    
    if (startPoly == endPoly)
    {
        // we are already on the same poly, so the path is solved
        NavPath_AddNode(outPath, startPoint, -1, startPoly->index);
        NavPath_AddNode(outPath, endPoint, -1, startPoly->index);
        return 1;
    }
    
    // reset count to 0
    stb__sbn(nav->pool) = 0;
    memset(nav->closed, 0, sizeof(char) * stb__sbn(nav->closed));
    
    assert(stb__sbn(nav->closed) == mesh->polyCount);
    
    struct NavSearchNode* first = stb_sb_add(nav->pool, 1);
    first->next = -1;
    first->parent = -1;
    first->polyIndex = startPoly->index;
    first->edgeIndex = -1;
    first->cost = 0.0f;

    nav->head = 0;
    
    while (nav->head != -1)
    {
        // pull the first (lowest cost) node from the list
        struct NavSearchNode* currentNode = nav->pool + nav->head;
        
        Vec3 currentPoint = startPoint;
        if (currentNode->edgeIndex != -1)
        {
            const NavEdge* edge = mesh->edges + currentNode->edgeIndex;
            currentPoint = Vec3_Lerp(mesh->vertices[edge->vertices[0]], mesh->vertices[edge->vertices[1]], 0.5f);
        }
        
        // move from open list to closed
        nav->head = currentNode->next;
        currentNode->next = -1;
        nav->closed[currentNode->polyIndex] = 1;

        // we found the target
        if (currentNode->polyIndex == endPoly->index)
        {
            // current node is currently an edge of the destination poly
            
            // construct a path in reverse from end to start
            NavPath temp;
            NavPath_Init(&temp);
            
            // end point (into the middle of the poly)
            NavPath_AddNode(&temp, endPoint, -1, endPoly->index);
            
            // middle points
            const struct NavSearchNode* i = currentNode;
            
            while (i != NULL && i->polyIndex != startPoly->index)
            {
                assert(i->edgeIndex != -1);
                const NavEdge* edge = mesh->edges + i->edgeIndex;
                Vec3 p = Vec3_Lerp(mesh->vertices[edge->vertices[0]], mesh->vertices[edge->vertices[1]], 0.5f);
                NavPath_AddNode(&temp, p, i->edgeIndex, i->polyIndex);

                assert(i->parent != -1);
                i = nav->pool + i->parent;
            }
            
            // start point
            NavPath_AddNode(&temp, startPoint, -1, startPoly->index);
            NavPath_Reverse(&temp, outPath);
            return 1;
        }
        
        // traverse neighbor connections
        const NavPoly* currentPoly = mesh->polys + currentNode->polyIndex;
        
        for (int i = 0; i < currentPoly->edgeCount; ++i)
        {
            const NavEdge* edge = mesh->edges + currentPoly->edgeStart + i;
            
            if (edge->neighborIndex == -1)
                continue;
            
            const NavPoly* neighbor = mesh->polys + edge->neighborIndex;
            
            if (nav->closed[neighbor->index] == 1)
                continue; // this node has already been evaluated
            
            // find the node if it is already in the open list
            struct NavSearchNode* toInsert = NULL;
            
            if (nav->head != -1)
            {
                struct NavSearchNode* node = nav->pool + nav->head;
                struct NavSearchNode* previous = NULL;
                
                while (node)
                {
                    if (node->polyIndex == neighbor->index)
                    {
                        toInsert = node;
                        
                        if (previous)
                        {
                            previous->next = toInsert->next;
                        }
                        else
                        {
                            nav->head = toInsert->next;
                        }
                        break;
                    }
                    
                    previous = node;
                    
                    if (node->next != -1)
                    {
                        node = nav->pool + node->next;
                    }
                    else
                    {
                        node = NULL;
                    }
                }
            }

            // if we didn't find a slot, allocate from the pool
            if (!toInsert)
                toInsert = stb_sb_add(nav->pool, 1);
            
            Vec3 edgeCenter = Vec3_Lerp(mesh->vertices[edge->vertices[0]], mesh->vertices[edge->vertices[1]], 0.5f);
            
            float heuristic = Vec3_Dist(edgeCenter, endPoint) * 1.5f;
            toInsert->cost = currentNode->cost + Vec3_Dist(edgeCenter, currentPoint) + heuristic;

            toInsert->polyIndex = neighbor->index;
            toInsert->edgeIndex = currentPoly->edgeStart + i;
            toInsert->parent = (int)(currentNode - nav->pool);
            toInsert->next = -1;

            int toInsertIndex = (int)(toInsert - nav->pool);

            // sorted insertion
            if (nav->head == -1)
            {
                nav->head = toInsertIndex;
            }
            else
            {
                struct NavSearchNode* node = nav->pool + nav->head;
                struct NavSearchNode* previous = NULL;
                
                int inserted = 0;
                
                while (node)
                {
                    if (toInsert->cost < node->cost)
                    {
                        toInsert->next = (int)(node - nav->pool);
                        
                        if (previous)
                        {
                            previous->next = toInsertIndex;
                        }
                        else
                        {
                            nav->head = toInsertIndex;
                        }
                        inserted = 1;
                        break;
                    }
                    
                    previous = node;
                    
                    if (node->next != -1)
                    {
                        node = nav->pool + node->next;
                    }
                    else
                    {
                        node = NULL;
                    }
                }
                
                if (!inserted)
                {
                    assert(previous);
                    previous->next = toInsertIndex;
                }
            }
        }
    }

    return 0;
}


/* http://digestingduck.blogspot.com/2010/03/simple-stupid-funnel-algorithm.html */
/* http://www.koffeebird.com/2014/05/towards-modified-simple-stupid-funnel.html */

void NavSolver_SmoothPath(const NavMesh* mesh, NavPath* path, float radius)
{
    // nothing to smooth if there is one point in the path
    if (path->nodeCount <= 1)
        return;
    
    Vec3 portals[path->nodeCount * 2];
    int pk = 0;
    
    Vec3 start = path->nodes[0].point;
    Vec3 end = path->nodes[path->nodeCount - 1].point;
    
    // starting point (no edge width)
    portals[pk] = start;
    portals[pk+1] = start;
    pk += 2;
    
    for (int i = 1; i < path->nodeCount - 1; ++i)
    {
        // line edge intersection to find the correct edge
        const NavEdge* edge = mesh->edges + path->nodes[i].edgeIndex;
        
        Vec3 a = mesh->vertices[edge->vertices[0]];
        Vec3 b = mesh->vertices[edge->vertices[1]];

        Vec3 edgeVec = Vec3_Norm(Vec3_Sub(a, b));
        
        portals[pk] = Vec3_Add(b, Vec3_Scale(edgeVec, radius));
        portals[pk+1] = Vec3_Sub(a, Vec3_Scale(edgeVec, radius));
        
        pk += 2;
    }
    
    // ending point (no edge width)
    portals[pk] = end;
    portals[pk + 1] = end;
    pk+=2;
    
    int npts = 0;
    Vec3 portalApex, portalLeft, portalRight;
    int apexIndex = 0, leftIndex = 0, rightIndex = 0;
    
    portalApex = portals[0];
    portalLeft = portals[0];
    portalRight = portals[1];
    
    path->nodes[npts].point = start;
    ++npts;
    
    for (int i = 1; i <= path->nodeCount && npts < pk; ++i)
    {
        Vec3* left = portals + i * 2;
        Vec3* right = portals + i * 2 + 1;
        
        if (TriArea(&portalApex, &portalRight, right) <= 0.0f)
        {
            if (VecEqual(&portalApex, &portalRight) ||
                TriArea(&portalApex, &portalLeft, right) > 0.0f)
            {
                portalRight = *right;
                rightIndex = i;
            }
            else
            {
                path->nodes[npts].point = portalLeft;
                ++npts;
                
                portalApex = portalLeft;
                apexIndex = leftIndex;
                
                portalLeft = portalApex;
                portalRight = portalApex;
                leftIndex = apexIndex;
                rightIndex = apexIndex;
                
                i = apexIndex;
                continue;
            }
        }
        if (TriArea(&portalApex, &portalLeft, left) >= 0.0f)
        {
            if (VecEqual(&portalApex, &portalLeft) ||
                TriArea(&portalApex, &portalRight, left) < 0.0f)
            {
                portalLeft = *left;
                leftIndex = i;
            }
            else
            {
                path->nodes[npts].point = portalRight;
                ++npts;
                
                portalApex = portalRight;
                apexIndex = rightIndex;
                portalLeft = portalApex;
                portalRight = portalApex;
                leftIndex = apexIndex;
                rightIndex = apexIndex;
                
                i = apexIndex;
                continue;
            }
        }
    }
    
    path->nodes[npts].point = end;
    ++npts;
    
    path->nodeCount = npts;
}


