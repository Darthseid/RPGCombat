#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>
#include "rpg_system.h" 

// ==========================================
// Helper: Target Selection
// ==========================================
Combatant* selectTarget(Combatant* actor, const std::vector<Combatant*>& participants, bool enemiesOnly) {
    std::cout << "\nSelect Target (" << (enemiesOnly ? "Enemies" : "Allies") << "):\n";
    std::vector<Combatant*> validTargets;

    int index = 1;
    for (auto* p : participants) {
        if (p->isAlive()) {
            bool isSameTeam = (p->getTeam() == actor->getTeam());

            // If we want enemies only, skip team members
            if (enemiesOnly && isSameTeam) continue;

            // If we want allies only (beneficial actions), skip enemies
            if (!enemiesOnly && !isSameTeam) continue;

            std::cout << index << ". " << p->getName()
                << " [" << p->getTeam() << "] (HP: " << p->getHP() << ")\n";
            validTargets.push_back(p);
            index++;
        }
    }

    if (validTargets.empty()) {
        std::cout << "No valid targets found.\n";
        return nullptr;
    }

    std::cout << "Choice: ";
    int choice;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        std::cin.ignore(1000, '\n');
        return nullptr;
    }

    if (choice < 1 || choice > validTargets.size()) return nullptr;
    return validTargets[choice - 1];
}

Combatant* getNearestEnemy(Combatant* actor, const std::vector<Combatant*>& participants) {
    Combatant* nearest = nullptr;
    float minDistance = 9999.0f;
    std::string targetTeam = (actor->getTeam() == "Good Guys") ? "Bad Guys" : "Good Guys";

    for (auto* target : participants) {
        if (target->getTeam() == targetTeam && target->isAlive() && target->getX() != -1) {
            float dist = getDistance(actor->getX(), actor->getY(), target->getX(), target->getY());
            if (dist < minDistance) {
                minDistance = dist;
                nearest = target;
            }
        }
    }
    return nearest;
}

// ==========================================
// Logic: Turn Handlers
// ==========================================

void runHumanTurn(Combatant* actor, BattleManager& battle, Grid& grid) {
    bool turnComplete = false;
    while (!turnComplete) {
        std::cout << "\n[MENU] 1.Attack  2.Guard  3.Move  4.Wait  5.Spell  6.Item\nChoice: ";
        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            std::cout << "Invalid input.\n";
            continue;
        }

        if (choice == 1) { // ATTACK
            // Attack always targets enemies
            Combatant* target = selectTarget(actor, battle.getParticipants(), true);
            if (target && actor->attack(*target, grid)) {
                actor->addTicks(COST_ATTACK);
                turnComplete = true;
            }
        }
        else if (choice == 2) { // GUARD
            actor->guard();
            actor->addTicks(COST_GUARD);
            turnComplete = true;
        }
        else if (choice == 3) { // MOVE
            std::cout << "Direction (Press 1 to go South, 2 to go North, 3 to go East, 4 to go West.): ";
            int dir;
            if (!(std::cin >> dir)) {
                std::cin.clear();
                std::cin.ignore(1000, '\n');
                std::cout << "Invalid input.\n";
                continue;
            }
            int cost = 0;
            switch (dir) {
            case 1: cost = grid.moveCombatant(actor, 0, 1); break; // South is +Y
            case 2: cost = grid.moveCombatant(actor, 0, -1); break; // North is -Y
            case 3: cost = grid.moveCombatant(actor, 1, 0); break; // East is +X
            case 4: cost = grid.moveCombatant(actor, -1, 0); break; // West is -X
            default:
                std::cout << "Invalid direction.\n";
                continue;
            }
            if (cost > 0) {
                actor->addTicks(cost);
                turnComplete = true;
            }
            else {
                std::cout << "Cannot move in that direction.\n";
            }
        }
        else if (choice == 4) { // WAIT
            actor->addTicks(COST_MOVE_BASE);
            turnComplete = true;
        }
        else if (choice == 5) { // SPELL
            if (actor->getSpells().empty()) {
                std::cout << "No spells known!\n";
                continue;
            }
            std::cout << "Select Spell:\n";
            for (size_t i = 0; i < actor->getSpells().size(); ++i) {
                std::cout << (i + 1) << ". " << actor->getSpells()[i].name
                    << " (AOE: " << actor->getSpells()[i].aoe
                    << ", " << actor->getSpells()[i].category << ")\n";
            }
            int sIdx;
            if (!(std::cin >> sIdx) || sIdx < 1 || sIdx > static_cast<int>(actor->getSpells().size())) {
                std::cin.clear();
                std::cin.ignore(1000, '\n');
                std::cout << "Invalid spell selection.\n";
                continue;
            }
            sIdx--; // 0-based

            // Determine targeting based on category
            std::string cat = actor->getSpells()[sIdx].category;
            bool enemiesOnly = (cat == "Debuff");

            Combatant* target = selectTarget(actor, battle.getParticipants(), enemiesOnly);
            if (target && actor->castSpell(*target, sIdx, grid)) {
                actor->addTicks(COST_SPELL);
                turnComplete = true;
            }
        }
        else if (choice == 6) { // ITEM
            if (actor->getInventory().empty()) {
                std::cout << "Inventory empty!\n";
                continue;
            }
            std::cout << "Select Item:\n";
            for (size_t i = 0; i < actor->getInventory().size(); ++i) {
                const auto& item = actor->getInventory()[i];
                std::cout << (i + 1) << ". " << item.name << " (x" << item.quantity
                    << ", " << item.category << ")\n";
            }
            int iIdx;
            if (!(std::cin >> iIdx) || iIdx < 1 || iIdx > static_cast<int>(actor->getInventory().size())) {
                std::cin.clear();
                std::cin.ignore(1000, '\n');
                std::cout << "Invalid item selection.\n";
                continue;
            }
            iIdx--;

            // Determine targeting based on category
            std::string cat = actor->getInventory()[iIdx].category;
            // Assuming Debuffs are the only offensive items, everything else (Healing/Buff/RestoreMP) is friendly
            bool enemiesOnly = (cat == "Debuff");

            Combatant* target = selectTarget(actor, battle.getParticipants(), enemiesOnly);
            if (target && actor->useItem(*target, iIdx)) {
                actor->addTicks(COST_ITEM);
                turnComplete = true;
            }
        }
        else {
            std::cout << "Invalid input.\n";
            std::cin.clear();
            std::cin.ignore(1000, '\n');
        }
    }
}

void runAITurn(Combatant* actor, BattleManager& battle, Grid& grid) {
    std::cout << "(AI Thinking...)\n";
    Combatant* target = getNearestEnemy(actor, battle.getParticipants());

    if (!target) {
        std::cout << " >> AI has no targets. Waiting.\n";
        actor->addTicks(COST_MOVE_BASE);
        return;
    }

    // Priority 1: Cast Spell
    for (size_t i = 0; i < actor->getSpells().size(); ++i) {
        const auto& spell = actor->getSpells()[i];
        if (actor->getMP() >= spell.mpCost) {
            if (actor->checkRange(*target, spell.range)) {
                actor->castSpell(*target, i, grid);
                actor->addTicks(COST_SPELL);
                return;
            }
            else {
                int dx = target->getX() - actor->getX();
                int dy = target->getY() - actor->getY();
                int moveX = (std::abs(dx) > std::abs(dy)) ? ((dx > 0) ? 1 : -1) : 0;
                int moveY = (moveX == 0) ? ((dy > 0) ? 1 : -1) : 0;

                std::cout << " >> AI moving to spell range...\n";
                int cost = grid.moveCombatant(actor, moveX, moveY);
                if (cost == 0) cost = COST_MOVE_BASE;
                actor->addTicks(cost);
                return;
            }
        }
    }

    // Priority 2: Attack
    if (actor->checkRange(*target, actor->getWeapon().range)) {
        actor->attack(*target, grid);
        actor->addTicks(COST_ATTACK);
    }
    else {
        int dx = target->getX() - actor->getX();
        int dy = target->getY() - actor->getY();
        int moveX = (std::abs(dx) > std::abs(dy)) ? ((dx > 0) ? 1 : -1) : 0;
        int moveY = (moveX == 0) ? ((dy > 0) ? 1 : -1) : 0;

        std::cout << " >> AI moving to attack...\n";
        int cost = grid.moveCombatant(actor, moveX, moveY);
        if (cost == 0) cost = COST_MOVE_BASE;
        actor->addTicks(cost);
    }
}

void handleBrokenUnit(Combatant* actor, Grid& grid) {
    std::cout << " >> " << actor->getName() << " is BROKEN and panics!\n";
    int x = actor->getX();
    int y = actor->getY();
    int w = grid.getWidth();
    int h = grid.getHeight();

    int distN = y, distS = h - 1 - y, distW = x, distE = w - 1 - x;
    int dx = 0, dy = -1, minDist = distN;

    if (distS < minDist) { minDist = distS; dx = 0; dy = 1; }
    if (distW < minDist) { minDist = distW; dx = -1; dy = 0; }
    if (distE < minDist) { minDist = distE; dx = 1; dy = 0; }

    int cost = grid.moveCombatant(actor, dx, dy);
    if (cost > 0 && !actor->hasFled()) actor->regainMorale(2);
    if (cost == 0) cost = COST_MOVE_BASE;
    actor->addTicks(cost);
}

// ==========================================
// Main Execution
// ==========================================

int main() {
    // 1. Items
    Item healthPotion("Health Potion", 1, 4.0f, "Healing", 50);
    Item magicPotion("Magic Potion", 1, 4.0f, "RestoreMP", 40);

    // 2. Weapons
    Weapon ironSword("Iron Sword", 50, 0.9f, 1.5f, 1, "Physical");
    Weapon woodenStaff("Wooden Staff", 20, 0.67f, 1.5f, 1, "Magical");
    Weapon woodBow("Wood Bow", 40, 0.5f, 11.0f, 1, "Physical");

    // 3. Armor
    Armor ironArmor("Iron Armor", 40, 0, 0.0f, "Standard", 0);
    Armor clothArmor("Cloth Armor", 10, 0, 0.0f, "Magical", 0);
    Armor woodenArmor("Wooden Armor", 20, 0, 0.0f, "Standard", 0);

    // 4. Spells (AOE 2, Debuff)
    // Name, Matk, Cost, Range, Duration, Element, AOE, Category
    Spell fireball("Fireball", 64, 15, 15.0f, 0, "Fire", 2, "Debuff");

    // 5. Combatants (Good Guys)
    auto dwayne = std::make_shared<Combatant>("Dwayne", "Good Guys", 200, 0, 5, 100);
    dwayne->equipWeapon(ironSword);
    dwayne->equipArmor(ironArmor);
    dwayne->addItem(healthPotion);

    auto elizabeth = std::make_shared<Combatant>("Elizabeth", "Good Guys", 100, 75, 7, 70);
    elizabeth->equipWeapon(woodenStaff);
    elizabeth->equipArmor(clothArmor);
    elizabeth->addItem(magicPotion);
    elizabeth->learnSpell(fireball);

    // 6. Combatants (Bad Guys)
    auto goblin1 = std::make_shared<Combatant>("Goblin Archer A", "Bad Guys", 90, 0, 9, 40);
    goblin1->equipWeapon(woodBow);
    goblin1->equipArmor(woodenArmor);

    auto goblin2 = std::make_shared<Combatant>("Goblin Archer B", "Bad Guys", 90, 0, 9, 40);
    goblin2->equipWeapon(woodBow);
    goblin2->equipArmor(woodenArmor);

    // 7. Grid Setup (12x12 to fit 10,0 and 0,10)
    Grid battleGrid(12, 12);
    battleGrid.placeCombatant(dwayne.get(), 0, 0);
    battleGrid.placeCombatant(elizabeth.get(), 2, 0);
    battleGrid.placeCombatant(goblin1.get(), 10, 0);
    battleGrid.placeCombatant(goblin2.get(), 0, 10);

    BattleManager battle;
    battle.addParticipant(dwayne.get());
    battle.addParticipant(elizabeth.get());
    battle.addParticipant(goblin1.get());
    battle.addParticipant(goblin2.get());

    // 8. Game Loop
    std::cout << "=== BATTLE START ===\n";
    std::cout << "Dwayne & Elizabeth vs Two Goblin Archers!\n";

    while (true) {
        // A. Victory Check
        std::string winner = battle.getWinner();
        if (winner != "None") {
            std::cout << "\n=================================\n";
            std::cout << "       " << winner << " TEAM WINS!       \n";
            std::cout << "=================================\n";
            break;
        }

        // B. Get Next Actor
        Combatant* actor = battle.getNextActiveCombatant();
        if (!actor) break;

        battleGrid.drawGrid();
        actor->startTurn();

        std::cout << "\n>>> TURN: " << actor->getName()
            << " (HP:" << actor->getHP() << " MP:" << actor->getMP() << ")\n";

        // C. Handle Turn Type
        if (actor->isBroken()) {
            handleBrokenUnit(actor, battleGrid);
        }
        else if (actor->getTeam() == "Good Guys") {
            runHumanTurn(actor, battle, battleGrid);
        }
        else {
            runAITurn(actor, battle, battleGrid);
        }
    }
    return 0;
}