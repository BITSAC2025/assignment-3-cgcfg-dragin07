/**
 * ICFG.cpp
 * @author kisslune 
 */

#include "CFGA.h"

using namespace SVF;
using namespace llvm;
using namespace std;

int main(int argc, char **argv)
{
    auto moduleNameVec =
            OptionBase::parseOptions(argc, argv, "Whole Program Points-to Analysis",
                                     "[options] <input-bitcode...>");

    LLVMModuleSet::buildSVFModule(moduleNameVec);

    SVFIRBuilder builder;
    auto pag = builder.build();
    auto icfg = pag->getICFG();

    CFGAnalysis analyzer = CFGAnalysis(icfg);

    // TODO: complete the following method: 'CFGAnalysis::analyze'
    analyzer.analyze(icfg);

    analyzer.dumpPaths();
    LLVMModuleSet::releaseLLVMModuleSet();
    return 0;
}


void CFGAnalysis::analyze(SVF::ICFG *icfg)
{
    // Sources and sinks are specified when an analyzer is instantiated.
    for (auto src : sources)
        for (auto snk : sinks)
        {
            // TODO: DFS the graph, starting from src and detecting the paths ending at snk.
            // Use the class method 'recordPath' (already defined) to record the path you detected.
            //@{
            while (!callStack.empty()) callStack.pop();

            std::vector<unsigned> path;
            std::set<std::pair<unsigned, std::vector<unsigned>>> visited;

            auto stackToVector = [&](const std::stack<unsigned>& st) -> std::vector<unsigned> {
                std::vector<unsigned> seq;
                std::stack<unsigned> tmp = st; 
                while (!tmp.empty()) { seq.push_back(tmp.top()); tmp.pop(); }
                std::reverse(seq.begin(), seq.end()); 
                return seq;
            };

            function<void(SVF::ICFGNode*)> dfs = [&](SVF::ICFGNode* node) {
                if (!node) return;

                unsigned nodeId = node->getId();
                auto stateSig = std::make_pair(nodeId, stackToVector(callStack));
                if (visited.find(stateSig) != visited.end())
                    return;
                visited.insert(stateSig);
                path.push_back(nodeId);

                // If we reached the sink and the call stack is empty, record this path
                if (nodeId == snk && callStack.empty())
                {
                    recordPath(path);
                    path.pop_back();
                    return;
                }

                // Traverse outgoing edges
                for (auto edge : node->getOutEdges())
                {
                    // Identify edge type
                    if (auto callEdge = dyn_cast<CallCFGEdge>(edge))
                    {
                        auto dst = callEdge->getDstNode();
                        auto callSite = callEdge->getCallSite();
                        bool didPush = false;
                        if (callSite) { callStack.push(callSite->getId()); didPush = true; }
                        dfs(dst);
                        if (didPush && !callStack.empty()) callStack.pop();
                    }
                    else if (auto retEdge = dyn_cast<RetCFGEdge>(edge))
                    {
                        auto dst = retEdge->getDstNode();
                        auto callSite = retEdge->getCallSite();
                        // Only follow return edges that match the most recent call
                        if (!callStack.empty() && callSite && callStack.top() == callSite->getId())
                        {
                            callStack.pop();
                            dfs(dst);
                            callStack.push(callSite->getId());
                        }
                        // else: unmatched return, skip
                    }
                    else
                    {
                        // Normal intra-procedural edge
                        auto dst = edge->getDstNode();
                        dfs(dst);
                    }
                }

                path.pop_back();
            };

            // Start DFS from source node
            dfs(icfg->getICFGNode(src));
            //@}
        }
}
