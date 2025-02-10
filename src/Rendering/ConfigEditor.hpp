// ConfigEditor.hpp
#pragma once
#include "Core/Config.hpp"
#include "Game/Game.hpp"
#include "imgui.h"

class ConfigEditor {
public:
  static void render(Game &game, Config &config, bool &show) {
    if (!show)
      return;

    ImGui::Begin("Config Editor", &show);

    if (ImGui::BeginTabBar("ConfigTabs")) {
      if (ImGui::BeginTabItem("Physics")) {
        ImGui::Text("Physics Settings");
        ImGui::Separator();

        ImGui::DragFloat("Gravity", &config.gravity, 1.0f, 0.0f, 2000.0f,
                         "%.1f");
        if (ImGui::IsItemHovered())
          ImGui::SetTooltip("Global gravity force applied to characters");

        ImGui::DragFloat("Friction", &config.friction, 0.1f, 0.0f, 10.0f,
                         "%.2f");
        if (ImGui::IsItemHovered())
          ImGui::SetTooltip("Ground friction coefficient");

        ImGui::DragFloat("Enemy Follow Force", &config.enemyFollowForce, 10.0f,
                         0.0f, 1000.0f);

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Ground & Movement")) {
        ImGui::Text("Ground Settings");
        ImGui::Separator();

        ImGui::DragInt("Ground Level", &config.groundLevel, 1.0f, 0, 2000);
        ImGui::DragInt("Ground Threshold", &config.groundThreshold, 1.0f, 1,
                       20);
        ImGui::DragInt("Stable Ground Frames", &config.stableGroundFrames, 1.0f,
                       1, 10);

        ImGui::Text("\nMovement Settings");
        ImGui::Separator();

        ImGui::DragFloat("Jump Velocity", &config.jumpVelocity, 10.0f, -5000.0f,
                         0.0f);
        ImGui::DragFloat("Move Force", &config.moveForce, 10.0f, 0.0f, 5000.0f);
        ImGui::DragFloat("Dash Force", &config.dashForce, 10.0f, 0.0f, 5000.0f);
        ImGui::DragFloat("Attack Force", &config.attackForce, 10.0f, 0.0f,
                         2000.0f);

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Combat")) {
        ImGui::Text("Combat Settings");
        ImGui::Separator();

        ImGui::DragFloat("Block Reduction", &config.baseBlockReduction, 0.01f,
                         0.0f, 1.0f, "%.2f");
        if (ImGui::IsItemHovered())
          ImGui::SetTooltip(
              "Damage multiplier when blocking (0 = full block, 1 = no block)");

        ImGui::DragFloat("Combo Scaling", &config.comboScaling, 0.05f, 1.0f,
                         2.0f, "%.2f");
        if (ImGui::IsItemHovered())
          ImGui::SetTooltip("Damage multiplier per combo hit");

        ImGui::DragFloat("Knockback Force", &config.knockbackForce, 10.0f, 0.0f,
                         2000.0f);
        ImGui::DragFloat("Knockback Combo Scaling",
                         &config.knockbackComboScaling, 0.01f, 0.0f, 1.0f,
                         "%.2f");

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("AI Settings")) {
        ImGui::Text("Zone Settings");
        ImGui::Separator();
        ImGui::DragFloat("Deadzone Boundary", &config.ai.deadzoneBoundary, 1.0f,
                         50.0f, 300.0f);
        if (ImGui::IsItemHovered())
          ImGui::SetTooltip("Distance from edges where penalties apply");
        ImGui::DragFloat("Optimal Distance", &config.ai.optimalDistance, 1.0f,
                         100.0f, 400.0f);

        ImGui::Spacing();
        ImGui::Text("Base Rewards");
        ImGui::Separator();
        ImGui::DragFloat("Health Diff Reward", &config.ai.healthDiffReward,
                         0.1f, 0.0f, 50.0f);
        ImGui::DragFloat("Hit Reward", &config.ai.hitReward, 0.1f, 0.0f, 50.0f);
        ImGui::DragFloat("Miss Penalty", &config.ai.missPenalty, 0.1f, -50.0f,
                         0.0f);
        ImGui::DragFloat("Block Reward", &config.ai.blockReward, 0.1f, 0.0f,
                         50.0f);
        ImGui::DragFloat("Block Penalty", &config.ai.blockPenalty, 0.1f, -50.0f,
                         0.0f);

        ImGui::Spacing();
        ImGui::Text("Position Rewards");
        ImGui::Separator();
        ImGui::DragFloat("Deadzone Base Penalty",
                         &config.ai.deadzoneBasePenalty, 0.1f, -100.0f, 0.0f);
        ImGui::DragFloat("Deadzone Depth Penalty",
                         &config.ai.deadzoneDepthPenalty, 0.1f, -100.0f, 0.0f);
        ImGui::DragFloat("Move Into Deadzone Penalty",
                         &config.ai.moveIntoDeadzonePenalty, 0.1f, -100.0f,
                         0.0f);
        ImGui::DragFloat("Escape Deadzone Reward",
                         &config.ai.escapeDeadzoneReward, 0.1f, 0.0f, 50.0f);
        ImGui::DragFloat("Distance Multiplier", &config.ai.distanceMultiplier,
                         0.001f, 0.0f, 1.0f);

        ImGui::Spacing();
        ImGui::Text("Combo System");
        ImGui::Separator();
        ImGui::DragFloat("Combo Base Multiplier",
                         &config.ai.comboBaseMultiplier, 0.01f, 0.0f, 2.0f);
        ImGui::DragFloat("Max Combo Multiplier", &config.ai.maxComboMultiplier,
                         0.1f, 1.0f, 10.0f);
        ImGui::DragFloat("Optimal Distance Bonus",
                         &config.ai.optimalDistanceBonus, 0.1f, 0.0f, 50.0f);
        ImGui::DragFloat("Far Whiff Penalty", &config.ai.farWhiffPenalty, 0.1f,
                         -50.0f, 0.0f);

        ImGui::Spacing();
        ImGui::Text("Stamina System");
        ImGui::Separator();
        ImGui::DragFloat("No Stamina Penalty", &config.ai.noStaminaPenalty,
                         0.1f, -100.0f, 0.0f);
        ImGui::DragFloat("Low Stamina Penalty", &config.ai.lowStaminaPenalty,
                         0.1f, -50.0f, 0.0f);
        ImGui::DragFloat("Low Stamina Threshold",
                         &config.ai.lowStaminaThreshold, 0.01f, 0.0f, 1.0f);

        ImGui::Spacing();
        ImGui::Text("Action Diversity");
        ImGui::Separator();
        ImGui::DragFloat("Repeat Action Penalty",
                         &config.ai.repeatActionPenalty, 0.1f, -50.0f, 0.0f);
        ImGui::DragFloat("Well Timed Block Bonus",
                         &config.ai.wellTimedBlockBonus, 0.1f, 0.0f, 50.0f);

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Camera")) {
        ImGui::Text("Camera Settings");
        ImGui::Separator();

        ImGui::DragFloat("Min Distance", &config.minDistance, 1.0f, 0.0f,
                         500.0f);
        ImGui::DragFloat("Max Distance", &config.maxDistance, 1.0f, 100.0f,
                         1000.0f);
        ImGui::DragFloat("Min Zoom", &config.minZoom, 0.1f, 0.1f, 1.0f, "%.2f");
        ImGui::DragFloat("Max Zoom", &config.maxZoom, 0.1f, 1.0f, 5.0f, "%.2f");
        ImGui::DragFloat("Camera Smooth Factor", &config.cameraSmoothFactor,
                         0.01f, 0.01f, 1.0f, "%.2f");

        ImGui::EndTabItem();
      }

      if (ImGui::CollapsingHeader("Training Settings")) {
        bool headless = game.isHeadlessMode();
        if (ImGui::Checkbox("Headless Mode", &headless)) {
          game.setHeadlessMode(headless);
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Run without rendering for maximum training speed");
        }
      }

      ImGui::EndTabBar();
    }

    ImGui::End();
  }
};
