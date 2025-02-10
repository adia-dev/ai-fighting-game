#pragma once
#include "AI/RLAgent.hpp"
#include "AI/State.hpp"
#include "imgui.h"

class DebugDraw {
public:
  static void DrawAIState(const RLAgent &agent) {
    ImGui::Begin("AI Debugger");

    if (!agent.m_qValueHistory.empty()) {
      ImGui::PlotLines("Q-Values", agent.m_qValueHistory.data(),
                       agent.m_qValueHistory.size());
    }

    const char *stanceStr = "";
    switch (agent.getCurrentStance()) {
    case Stance::Neutral:
      stanceStr = "Neutral";
      break;
    case Stance::Aggressive:
      stanceStr = "Aggressive";
      break;
    case Stance::Defensive:
      stanceStr = "Defensive";
      break;
    }
    ImGui::Text("Current Stance: %s", stanceStr);

    ImGui::SeparatorText("Action History");
    ImGui::Text("Self Actions:");
    for (auto action : agent.getActionHistory()) {
      ImGui::SameLine();
      ImGui::Text("%s", actionTypeToString(action));
    }
    ImGui::Text("Opponent Actions:");
    for (auto action : agent.getOpponentActionHistory()) {
      ImGui::SameLine();
      ImGui::Text("%s", actionTypeToString(action));
    }

    ImGui::End();
  }
};
