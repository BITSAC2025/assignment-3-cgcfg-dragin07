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
            // 清空调用栈
            while (!callStack.empty()) callStack.pop();

            std::vector<unsigned> path;
            // 访问状态：节点 ID + 当前调用栈内容
            std::set<std::pair<unsigned, std::vector<unsigned>>> visited;

            // 将调用栈拷贝为 vector（栈底在前）
            auto stackToVector = [&](const std::stack<unsigned> &st) -> std::vector<unsigned> {
                std::vector<unsigned> seq;
                std::stack<unsigned> tmp = st;
                while (!tmp.empty())
                {
                    seq.push_back(tmp.top());
                    tmp.pop();
                }
                std::reverse(seq.begin(), seq.end());
                return seq;
            };

            std::function<void(SVF::ICFGNode *)> dfs = [&](SVF::ICFGNode *node) {
                if (!node) return;

                unsigned nodeId = node->getId();
                auto stateSig = std::make_pair(nodeId, stackToVector(callStack));
                if (visited.find(stateSig) != visited.end())
                    return;
                visited.insert(stateSig);

                path.push_back(nodeId);

                // 到达 sink 且调用栈为空，记录一条完整路径
                if (nodeId == snk && callStack.empty())
                {
                    recordPath(path);
                    path.pop_back();
                    return;
                }

                // 遍历后继边
                for (auto edge : node->getOutEdges())
                {
                    // 调用边：callsite -> 被调函数入口
                    if (auto callEdge = dyn_cast<CallCFGEdge>(edge))
                    {
                        auto dst      = callEdge->getDstNode();
                        auto callSite = callEdge->getCallSite();
                        bool pushed   = false;

                        if (callSite)
                        {
                            callStack.push(callSite->getId());
                            pushed = true;
                        }

                        dfs(dst);

                        if (pushed && !callStack.empty())
                            callStack.pop();
                    }
                    // 返回边：被调函数出口 -> 调用点之后
                    else if (auto retEdge = dyn_cast<RetCFGEdge>(edge))
                    {
                        auto dst      = retEdge->getDstNode();
                        auto callSite = retEdge->getCallSite();

                        if (!callSite)
                        {
                            // 没有 callsite 信息，当作普通边处理
                            dfs(dst);
                        }
                        else if (!callStack.empty() && callStack.top() == callSite->getId())
                        {
                            // 与当前栈顶的调用点匹配，正常的 call-return
                            callStack.pop();
                            dfs(dst);
                            callStack.push(callSite->getId());
                        }
                        else if (callStack.empty())
                        {
                            // 从被调函数内部作为源点开始 DFS，没有调用上下文；
                            // 允许沿所有返回边回到可能的调用点，但不修改空栈。
                            dfs(dst);
                        }
                        // 否则：调用栈非空但与栈顶不匹配，认为是“不可能的返回”，跳过
                    }
                    // 普通 CFG 边（函数内部）
                    else
                    {
                        auto dst = edge->getDstNode();
                        dfs(dst);
                    }
                }

                path.pop_back();
            };

            // 从 source 对应的 ICFG 节点开始 DFS
            dfs(icfg->getICFGNode(src));
            //@}
        }
}
