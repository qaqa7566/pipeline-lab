#ifndef PLAB_VIZ_TRACE_HPP
#define PLAB_VIZ_TRACE_HPP

#include <string>

#include "plab/sim/pipeline.hpp"

namespace plab::viz {

/// Renders the cycle-by-cycle pipeline diagram: one row per fetched
/// instruction, one column per cycle, each cell showing the stage
/// (IF/ID/EX/MEM/WB) that instruction occupied. Repeated ID cells indicate a
/// stall; rows that only reach IF were squashed by a flush.
std::string renderPipelineDiagram(const sim::Pipeline& pipeline);

/// Renders the performance summary (cycles, retired, CPI/IPC, stalls, flushes,
/// forwards).
std::string renderPerformanceReport(const sim::PipelineStats& stats);

}  // namespace plab::viz

#endif  // PLAB_VIZ_TRACE_HPP
