#include "halfedge.h"

#include <set>
#include <map>
#include <vector>
#include <string>

#include <Eigen/Core>
#include <Eigen/Dense>
#include <spdlog/spdlog.h>

using Eigen::Matrix3f;
using Eigen::Matrix4f;
using Eigen::Vector3f;
using Eigen::Vector4f;
using std::optional;
using std::set;
using std::size_t;
using std::string;
using std::unordered_map;
using std::vector;

HalfedgeMesh::EdgeRecord::EdgeRecord(unordered_map<Vertex*, Matrix4f>& vertex_quadrics, Edge* e)
    : edge(e)
{
    (void)vertex_quadrics;
    optimal_pos = Vector3f(0.0f, 0.0f, 0.0f);
    cost        = 0.0f;
}

bool operator<(const HalfedgeMesh::EdgeRecord& a, const HalfedgeMesh::EdgeRecord& b)
{
    return a.cost < b.cost;
}

optional<Edge*> HalfedgeMesh::flip_edge(Edge* e)    //要求考虑传入的边在 mesh 边界上的情况 边翻转
{   
    /*
    (void)e;
    return std::nullopt;
    */
    if (!e->on_boundary()) {
        // 要用到的半边
        Halfedge* h = e->halfedge;
        Halfedge* h_inv = h->inv;
        Halfedge* h_2_3 = h->next;
        Halfedge* h_3_1 = h_2_3->next;
        Halfedge* h_1_4 = h_inv->next;
        Halfedge* h_4_2 = h_1_4->next;
        // 要用到的顶点
        // v1 and v2 are vertices along the edge
        Vertex* v1 = h->from;
        Vertex* v2 = h_inv->from;
        // v3 and v4 are vertices opposite the edge
        Vertex* v3 = h_3_1->from;
        Vertex* v4 = h_4_2->from;
        // 要用到的面片
        Face* f1 = h->face;
        Face* f2 = h_inv->face;
        // 重新连接各基本元素
        //h->set_neighbors(next, prev, inv, from, edge, face);
        h->set_neighbors(h_3_1, h_1_4, h_inv, v4, e, f1);
        h_inv->set_neighbors(h_4_2, h_2_3, h, v3, e, f2);
        h_2_3->set_neighbors(h_inv, h_4_2, h_2_3->inv, v2, h_2_3->edge, h_2_3->face);
        h_3_1->set_neighbors(h_1_4, h, h_3_1->inv, v3, h_3_1->edge, h_3_1->face);
        h_1_4->set_neighbors(h, h_3_1, h_1_4->inv, v1, h_1_4->edge, h_1_4->face);
        h_4_2->set_neighbors(h_2_3, h_inv, h_4_2->inv, v4, h_4_2->edge, h_4_2->face);
        return e;
    }
    (void)e;
    return std::nullopt;
}

optional<Vertex*> HalfedgeMesh::split_edge(Edge* e)     //不要求考虑边界边 边分裂  
{                                                       //考虑对半边的处理错误，对于点、边、面的初始化没有问题        
    // 要用到的半边
    if (!e->on_boundary()) {
        Halfedge* h = e->halfedge;
        Halfedge* h_inv = h->inv;
        Halfedge* h_2_3 = h->next;
        Halfedge* h_3_1 = h_2_3->next;
        Halfedge* h_1_4 = h_inv->next;
        Halfedge* h_4_2 = h_1_4->next;
        // 要用到的顶点
        // v1 and v2 are vertices along the edge
        Vertex* v1 = h->from;
        Vertex* v2 = h_inv->from;
        // v3 and v4 are vertices opposite the edge
        Vertex* v3 = h_3_1->from;
        Vertex* v4 = h_4_2->from;
        // 要用到的面片
        Face* f1 = h->face;
        Face* f2 = h_inv->face;
        //生成新产生的元素
        Vertex* v5 = new_vertex();
        v5->pos = e->center();

        Halfedge* h_5_1 = new_halfedge();
        Halfedge* h_5_2 = new_halfedge();
        Halfedge* h_3_5 = new_halfedge();
        Halfedge* h_5_3 = new_halfedge();
        Halfedge* h_5_4 = new_halfedge();
        Halfedge* h_4_5 = new_halfedge();

        Edge* e1 = new_edge();
        Edge* e2 = new_edge();
        Edge* e3 = new_edge();

        Face* f3 = new_face();
        Face* f4 = new_face();
        //面、点、边只维护一条半边
        e1->halfedge = h_4_5;
        e2->halfedge = h_inv;
        e3->halfedge = h_3_5;
        f1->halfedge = h_3_1;
        f2->halfedge = h_4_2;
        f3->halfedge = h_5_1;
        f4->halfedge = h_5_2;
        v5->halfedge = h_5_1;

        //重新定义半边的参数 h->set_neighbors(next, prev, inv, from, edge, face);
        h->set_neighbors(h_5_3, h_3_1, h_5_1, v1, e, f1);
        h_5_3->set_neighbors(h_3_1, h, h_3_5, v5, e3, f1);
        h_3_1->set_neighbors(h, h_5_3, h_3_1->inv, v3, h_3_1->edge, f1);

        h_inv->set_neighbors(h_5_4, h_4_2, h_5_2, v2, e2, f2);
        h_5_4->set_neighbors(h_4_2, h_inv, h_4_5, v5, e1, f2);
        h_4_2->set_neighbors(h_inv, h_5_4, h_4_2->inv, v4, h_4_2->edge, f2);

        h_5_1->set_neighbors(h_1_4, h_4_5, h, v5, e, f3);
        h_1_4->set_neighbors(h_4_5, h_5_1, h_1_4->inv, v1, h_1_4->edge, f3);
        h_4_5->set_neighbors(h_5_1, h_1_4, h_5_4, v4, e1, f3);

        h_3_5->set_neighbors(h_5_2, h_2_3, h_5_3, v3, e3, f4);
        h_5_2->set_neighbors(h_2_3, h_3_5, h_inv, v5, e2, f4);
        h_2_3->set_neighbors(h_3_5, h_5_2, h_2_3->inv, v2, h_2_3->edge, f4);

        return v5;
    }
    return std::nullopt;
}

optional<Vertex*> HalfedgeMesh::collapse_edge(Edge* e)      //要求考虑传入的边在 mesh 边界上的情况 边坍缩
{                                                           //对比了一下完整版的，考虑一下边删除不完全的问题
    if (e->on_boundary()) {
        //初始化6条半边
        Halfedge* h_1_2 = e->halfedge;
        Halfedge* h_2_3 = h_1_2->next;
        Halfedge* h_3_1 = h_2_3->next;
        Halfedge* h_2_1 = h_1_2->inv;
        Halfedge* h_3_2 = h_2_3->inv;
        Halfedge* h_1_3 = h_3_1->inv;
        //初始化三个边
        Edge* e1 = h_3_2->edge;
        Edge* e2 = h_3_1->edge;
        //初始化3个顶点
        Vertex* v1 = h_1_2->from;
        Vertex* v2 = h_2_3->from;
        Vertex* v3 = h_3_1->from;
        //初始化一个面
        Face* f1 = h_1_2->face;
        //生成新的元素
        Edge* E1 = new_edge();
        //维护新的元素
        v1->halfedge = h_1_3;
        E1->halfedge = h_1_3;
        //维护保存的半边 h->set_neighbors(next, prev, inv, from, edge, face);
        h_1_3->set_neighbors(h_1_3->next, h_1_3->prev, h_3_2, v1, E1, h_1_3->face);
        h_3_2->set_neighbors(h_3_2->next, h_3_2->prev, h_1_3, v3, E1, h_3_2->face);
        
        h_3_2->next->from = v1;
        h_2_1->next->from = v1;
        
        Halfedge* H = h_2_1;
        while (H->next->inv->next != h_1_3) {
            H->next->from = v1;
            H->next->inv->next->from = v1;
            H = H->next->inv;
        }
        H = h_1_2;
        while (H->next->inv->next != h_2_3) {
            H->next->from = v1;
            H->next->inv->next->from = v1;
            H = H->next->inv;
        }
        v1->pos = e->center();
        //删除旧的
        erase(h_3_1);
        erase(h_2_3);
        erase(h_1_2);
        erase(h_2_1);
        erase(v2);
        erase(e);
        erase(e1);
        erase(e2);
        erase(f1);
        return v1;
    }
    else {          //没问题了
        //初始10条半边
        Halfedge* h_1_2 = e->halfedge;
        Halfedge* h_2_1 = h_1_2->inv;
        Halfedge* h_2_3 = h_1_2->next;
        Halfedge* h_3_1 = h_2_3->next;
        Halfedge* h_3_2 = h_2_3->inv;
        Halfedge* h_1_3 = h_3_1->inv;
        Halfedge* h_1_4 = h_2_1->next;
        Halfedge* h_4_2 = h_1_4->next;
        Halfedge* h_4_1 = h_1_4->inv;
        Halfedge* h_2_4 = h_4_2->inv;
        //初始4个顶点
        Vertex* v1 = h_1_2->from;
        Vertex* v2 = h_2_1->from;
        Vertex* v3 = h_3_1->from;
        Vertex* v4 = h_4_2->from;
        //初始4+1条边
        Edge* e1 = h_1_2->edge;
        Edge* e2 = h_3_2->edge;
        Edge* e3 = h_2_4->edge;
        Edge* e4 = h_1_4->edge;
        //初始两个面片
        Face* f1 = h_1_2->face;
        Face* f2 = h_2_1->face;
        //生成新的元素
        Vertex* v5 = new_vertex();
        //维护新的元素
        v3->halfedge = h_3_2;
        v4->halfedge = h_4_1;
        v5->halfedge = h_1_3;
        v5->pos = e->center();
        //h->set_neighbors(next, prev, inv, from, edge, face);
        h_1_3->set_neighbors(h_1_3->next, h_1_3->prev, h_3_2, v5, e1, h_1_3->face);
        h_3_2->set_neighbors(h_3_2->next, h_3_2->prev, h_1_3, v3, e1, h_3_2->face);
        h_2_4->set_neighbors(h_2_4->next, h_2_4->prev, h_4_1, v5, e3, h_2_4->face);
        h_4_1->set_neighbors(h_4_1->next, h_4_1->prev, h_2_4, v4, e3, h_4_1->face);
        
        h_4_1->next->from = v5;
        h_3_2->next->from = v5;
        h_3_2->next->inv->next->from = v5;
        h_4_1->next->inv->next->from = v5;
        
        Halfedge* H = h_4_1;
        while (H->next->inv->next != h_1_3) {
            H->next->from = v5;
            H->next->inv->next->from = v5;
            H = H->next->inv;
        }
        H = h_3_2;
        while (H->next->inv->next != h_2_4) {
            H->next->from = v5;
            H->next->inv->next->from = v5;
            H = H->next->inv;
        }
        //删除旧的元素
        erase(h_3_1);
        erase(h_2_3);
        erase(h_1_2);
        erase(h_2_1);
        erase(h_1_4);
        erase(h_4_2);
        erase(v1);
        erase(v2);
        erase(f1);
        erase(f2);
        erase(e);
        erase(e2);
        erase(e4);
        erase(H);
        return v5;
    }
}

void HalfedgeMesh::loop_subdivide()
{
    optional<HalfedgeMeshFailure> check_result = validate();
    if (check_result.has_value()) {
        return;
    }
    logger->info("subdivide object {} (ID: {}) with Loop Subdivision strategy", object.name,
                 object.id);
    logger->info("original mesh: {} vertices, {} faces in total", vertices.size, faces.size);
    // Each vertex and edge of the original mesh can be associated with a vertex
    // in the new (subdivided) mesh.
    // Therefore, our strategy for computing the subdivided vertex locations is to
    // *first* compute the new positions using the connectivity of the original
    // (coarse) mesh. Navigating this mesh will be much easier than navigating
    // the new subdivided (fine) mesh, which has more elements to traverse.
    // We will then assign vertex positions in the new mesh based on the values
    // we computed for the original mesh.

    // Compute new positions for all the vertices in the input mesh using
    // the Loop subdivision rule and store them in Vertex::new_pos.
    //    At this point, we also want to mark each vertex as being a vertex of the
    //    original mesh. Use Vertex::is_new for this.

    // Next, compute the subdivided vertex positions associated with edges, and
    // store them in Edge::new_pos.

    // Next, we're going to split every edge in the mesh, in any order.
    // We're also going to distinguish subdivided edges that came from splitting
    // an edge in the original mesh from new edges by setting the boolean Edge::is_new.
    // Note that in this loop, we only want to iterate over edges of the original mesh.
    // Otherwise, we'll end up splitting edges that we just split (and the
    // loop will never end!)
    // I use a vector to store iterators of original because there are three kinds of
    // edges: original edges, edges split from original edges and newly added edges.
    // The newly added edges are marked with Edge::is_new property, so there is not
    // any other property to mark the edges I just split.

    // Now flip any new edge that connects an old and new vertex.

    // Finally, copy new vertex positions into the Vertex::pos.

    // Once we have successfully subdivided the mesh, set global_inconsistent
    // to true to trigger synchronization with GL::Mesh.
    global_inconsistent = true;
    logger->info("subdivided mesh: {} vertices, {} faces in total", vertices.size, faces.size);
    logger->info("Loop Subdivision done");
    logger->info("");
    validate();
}

void HalfedgeMesh::simplify()
{
    optional<HalfedgeMeshFailure> check_result = validate();
    if (check_result.has_value()) {
        return;
    }
    logger->info("simplify object {} (ID: {})", object.name, object.id);
    logger->info("original mesh: {} vertices, {} faces", vertices.size, faces.size);
    unordered_map<Vertex*, Matrix4f> vertex_quadrics;
    unordered_map<Face*, Matrix4f> face_quadrics;
    unordered_map<Edge*, EdgeRecord> edge_records;
    set<EdgeRecord> edge_queue;

    // Compute initial quadrics for each face by simply writing the plane equation
    // for the face in homogeneous coordinates. These quadrics should be stored
    // in face_quadrics

    // -> Compute an initial quadric for each vertex as the sum of the quadrics
    //    associated with the incident faces, storing it in vertex_quadrics

    // -> Build a priority queue of edges according to their quadric error cost,
    //    i.e., by building an Edge_Record for each edge and sticking it in the
    //    queue. You may want to use the above PQueue<Edge_Record> for this.

    // -> Until we reach the target edge budget, collapse the best edge. Remember
    //    to remove from the queue any edge that touches the collapsing edge
    //    BEFORE it gets collapsed, and add back into the queue any edge touching
    //    the collapsed vertex AFTER it's been collapsed. Also remember to assign
    //    a quadric to the collapsed vertex, and to pop the collapsed edge off the
    //    top of the queue.

    logger->info("simplified mesh: {} vertices, {} faces", vertices.size, faces.size);
    logger->info("simplification done\n");
    global_inconsistent = true;
    validate();
}

void HalfedgeMesh::isotropic_remesh()
{
    optional<HalfedgeMeshFailure> check_result = validate();
    if (check_result.has_value()) {
        return;
    }
    logger->info("remesh the object {} (ID: {}) with strategy Isotropic Remeshing", object.name,
                 object.id);
    logger->info("original mesh: {} vertices, {} faces", vertices.size, faces.size);
    // Compute the mean edge length.

    // Repeat the four main steps for 5 or 6 iterations
    // -> Split edges much longer than the target length (being careful about
    //    how the loop is written!)
    // -> Collapse edges much shorter than the target length.  Here we need to
    //    be EXTRA careful about advancing the loop, because many edges may have
    //    been destroyed by a collapse (which ones?)
    // -> Now flip each edge if it improves vertex degree
    // -> Finally, apply some tangential smoothing to the vertex positions
    static const size_t iteration_limit = 5;
    set<Edge*> selected_edges;
    for (size_t i = 0; i != iteration_limit; ++i) {
        // Split long edges.

        // Collapse short edges.

        // Flip edges.

        // Vertex averaging.
    }
    logger->info("remeshed mesh: {} vertices, {} faces\n", vertices.size, faces.size);
    global_inconsistent = true;
    validate();
}
