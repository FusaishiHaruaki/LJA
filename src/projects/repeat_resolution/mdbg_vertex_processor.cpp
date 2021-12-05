//
// Created by Andrey Bzikadze on 11/25/21.
//

#include "mdbg_vertex_processor.hpp"

using namespace repeat_resolution;

void MDBGSimpleVertexProcessor::Process0In1Pout(MultiplexDBG &graph,
                                                  const RRVertexType &vertex) {
  RRVertexProperty &v_prop = graph.node_prop(vertex);
  auto [out_nbr_begin, out_nbr_end] = graph.out_neighbors(vertex);
  for (auto it = out_nbr_begin; it != out_nbr_end; ++it) {
    RRVertexType new_vertex = graph.GetNewVertex(v_prop.Seq());
    graph.MoveEdge(vertex, it, new_vertex, it->first);
    graph.IncreaseVertex(new_vertex, 1);
  }
  graph.remove_nodes(vertex); // careful: Iterator is invalidated
}

void MDBGSimpleVertexProcessor::Process1Pin0Out(MultiplexDBG &graph,
                                                  const RRVertexType &vertex) {
  RRVertexProperty &v_prop = graph.node_prop(vertex);
  auto [in_nbr_begin, in_nbr_end] = graph.in_neighbors(vertex);
  for (auto it = in_nbr_begin; it != in_nbr_end; ++it) {
    EdgeIndexType edge_index = it->second.prop().Index();
    RRVertexType new_vertex = graph.GetNewVertex(v_prop.Seq());
    // need to construct a NeighborIterator pointing to vertex
    auto out_nbr = graph.out_neighbors(it->first).first;
    while (out_nbr->second.prop().Index() != edge_index) {
      ++out_nbr;
    }
    graph.MoveEdge(it->first, out_nbr, it->first, new_vertex);
    graph.IncreaseVertex(new_vertex, 1);
  }
  graph.remove_nodes(vertex); // careful: Iterator is invalidated
}

void MDBGSimpleVertexProcessor::Process(MultiplexDBG &graph,
                                        const RRVertexType &vertex,
                                        uint64_t n_iter) {
  const int indegree = graph.count_in_neighbors(vertex);
  const int outdegree = graph.count_out_neighbors(vertex);
  VERIFY(indegree < 2 or outdegree < 2);

  VERIFY_MSG(indegree != 1 or outdegree != 1,
             "no vertexes on nonbranching paths allowed");
  RRVertexProperty &v_prop = graph.node_prop(vertex);
  if (indegree == 0 and outdegree == 0) {
    // Isolates should be skipped
  } else if (indegree == 0 and outdegree == 1) {
    // tip. Only increment length
    graph.IncreaseVertex(vertex, n_iter);
  } else if (indegree == 1 and outdegree == 0) {
    // tip. Only increment length
    graph.IncreaseVertex(vertex, n_iter);
  } else if (indegree == 0 and outdegree > 1) {
    // "Starting" vertex
    VERIFY(n_iter == 1);
    Process0In1Pout(graph, vertex);

  } else if (indegree > 1 and outdegree == 0) {
    // "Finishing" vertex
    VERIFY(n_iter == 1);
    Process1Pin0Out(graph, vertex);

  } else if (indegree == 1 and outdegree > 1) {
    graph.IncreaseVertex(vertex, n_iter);

  } else if (indegree > 1 and outdegree == 1) {
    graph.IncreaseVertex(vertex, n_iter);
  }
}

std::pair<std::unordered_map<EdgeIndexType, RRVertexType>,
          std::vector<RRVertexType>>
MDBGComplexVertexProcessor::SplitVertex(MultiplexDBG &graph,
                                         const RRVertexType &vertex) {
  RRVertexProperty &v_prop = graph.node_prop(vertex);
  std::unordered_map<EdgeIndexType, RRVertexType> edge2vertex;
  std::vector<RRVertexType> new_vertices;
  auto [in_nbr_begin, in_nbr_end] = graph.in_neighbors(vertex);
  for (auto it = in_nbr_begin; it != in_nbr_end; ++it) {
    const RRVertexType &neighbor = it->first;
    const EdgeIndexType edge_index = it->second.prop().Index();
    RRVertexType new_vertex = graph.GetNewVertex(v_prop.Seq());
    new_vertices.emplace_back(new_vertex);
    auto e_it = graph.out_neighbors(neighbor).first;
    while (e_it->second.prop().Index() != edge_index) {
      ++e_it;
    }
    graph.MoveEdge(neighbor, e_it, neighbor, new_vertex);
    graph.IncreaseVertex(new_vertex, 1);
    edge2vertex.emplace(edge_index, neighbor);
  }

  auto [out_nbr_begin, out_nbr_end] = graph.out_neighbors(vertex);
  for (auto it = out_nbr_begin; it != out_nbr_end; ++it) {
    const EdgeIndexType edge_index = it->second.prop().Index();
    RRVertexType new_vertex = graph.GetNewVertex(v_prop.Seq());
    new_vertices.emplace_back(new_vertex);
    graph.MoveEdge(vertex, it, new_vertex, it->first);
    graph.IncreaseVertex(new_vertex, 1);
    // here we use map[key] = value instead of map.emplace(key, value)
    // because edge_index is already saved in the dict if the edge is a loop
    edge2vertex[edge_index] = new_vertex;
  }
  return {edge2vertex, new_vertices};
}

void MDBGComplexVertexProcessor::Process(MultiplexDBG &graph,
                                         const RRVertexType &vertex) {
  const RRVertexProperty &v_prop = graph.node_prop(vertex);

  auto [ac_s2e, ac_e2s] = graph.GetEdgepairsVertex(vertex);
  auto [edge2vertex, new_vertices] = SplitVertex(graph, vertex);

  for (const auto &[edge1, edge1_neighbors] : ac_s2e) {
    for (const auto &edge2 : edge1_neighbors) {
      // const EdgeIndexType edge1 = FindMergeEdgeId(edge1_);
      const RRVertexType left_vertex = edge2vertex.at(edge1);
      auto e1_it = graph.FindOutEdgeIterator(left_vertex, edge1);

      // const EdgeIndexType edge2 = FindMergeEdgeId(edge2_);
      const RRVertexType right_vertex = edge2vertex.at(edge2);
      const std::unordered_set<EdgeIndexType> &edge2_neighbors = ac_e2s[edge2];
      auto e2_it = graph.FindOutEdgeIterator(right_vertex, edge2);

      graph.AddConnectingEdge(e1_it, right_vertex, e2_it);
    }
  }
  for (const RRVertexType &new_vertex : new_vertices) {
    const uint64_t indegree = graph.count_in_neighbors(new_vertex);
    const uint64_t outdegree = graph.count_out_neighbors(new_vertex);
    if (indegree == 1 and outdegree == 1) {
      auto in_rev_it = graph.in_neighbors(new_vertex).first;
      const RRVertexType &left_vertex = in_rev_it->first;
      if (left_vertex == new_vertex) {
        // self-loop should be skipped
        continue;
      }
      auto in_it = graph.FindOutEdgeIterator(left_vertex,
                                             in_rev_it->second.prop().Index());
      auto out_it = graph.out_neighbors(new_vertex).first;
      graph.MergeEdges(left_vertex, in_it, out_it);
    }
  }
  graph.remove_nodes(vertex);
}