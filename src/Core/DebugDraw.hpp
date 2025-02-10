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

    if (ImGui::CollapsingHeader("Self Actions")) {
      Action lastAction = agent.lastAction();
      ImGui::Text("Last Action: %s", actionTypeToString(lastAction.type));

      for (auto action : agent.getActionHistory()) {
        ImGui::Text("%s", actionTypeToString(action));
      }
    }
    if (ImGui::CollapsingHeader("Opponent Actions:")) {
      for (auto action : agent.getOpponentActionHistory()) {
        ImGui::Text("%s", actionTypeToString(action));
      }
    }

    ImGui::End();
  }
};
