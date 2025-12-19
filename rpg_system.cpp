#include "rpg_system.h"
#include <random>
#include <algorithm> 
#include <limits>    
#include <cmath>     

// Helper functions for Random Number Generation
int getRandomInt(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

float getRandomFloat(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(min, max);
    return dis(gen);
}

float getDistance(int x1, int y1, int x2, int y2) {
    return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

// ==========================================
// Elemental Relationships
// ==========================================
float getElementalMultiplier(const std::string& atk, const std::string& def) {
    if (atk == def && atk != "None" && atk != "") return 0.5f;

    if ((atk == "Fire" && def == "Ice") || (atk == "Ice" && def == "Fire")) return 2.0f;
    if ((atk == "Psi" && def == "Bio") || (atk == "Bio" && def == "Psi")) return 2.0f;
    if ((atk == "Electricity" && def == "Earth") || (atk == "Earth" && def == "Electricity")) return 2.0f;
    if ((atk == "Wind" && def == "Poison") || (atk == "Poison" && def == "Wind")) return 2.0f;

    return 1.0f;
}

// ==========================================
// Item Implementation
// ==========================================
Item::Item(std::string n, int qty, float rng, std::string cat, int pot)
    : name(n), quantity(qty), range(rng), category(cat), potency(pot) {
}

// ==========================================
// Armor Implementation
// ==========================================
Armor::Armor(std::string n, int dr, int dt, float eva, std::string elem, int magDef)
    : name(n), damageResistance(dr), damageThreshold(dt),
    evasion(eva), elementType(elem), magicalDefense(magDef)
{
    if (elementType != "Wind") this->evasion = 0.0f;
    if (elementType != "Earth") this->damageThreshold = 0;
    if (elementType != "Naughtium") this->magicalDefense = 0;
}

Armor::Armor()
    : name("Naked"), damageResistance(0), damageThreshold(0), evasion(0), elementType("Standard"), magicalDefense(0) {
}

// ==========================================
// Weapon Implementation
// ==========================================
Weapon::Weapon(std::string n, int atk, float acc, float rng, int num, std::string elem)
    : name(n), physicalAttack(atk), accuracy(acc), range(rng), numberOfAttacks(num), elementType(elem) {
}

Weapon::Weapon()
    : name("Fists"), physicalAttack(1), accuracy(1.0f), range(1.0f), numberOfAttacks(1), elementType("Physical") {
}

// ==========================================
// Spell Implementation
// ==========================================
// Updated Constructor
Spell::Spell(std::string n, int matk, int cost, float rng, int dur, std::string elem, int area, std::string cat)
    : name(n), magicalAttack(matk), mpCost(cost), range(rng), duration(dur), elementType(elem), aoe(area), category(cat) {
}

// ==========================================
// Combatant Implementation
// ==========================================
Combatant::Combatant(std::string n, std::string teamName, int hp, int mp, int init, int mor)
    : name(n), team(teamName), maxHealth(hp), currentHealth(hp),
    maxMagicPoints(mp), currentMagicPoints(mp),
    initiative(init), morale(mor) {
}

std::string Combatant::getName() const { return name; }
std::string Combatant::getTeam() const { return team; }
int Combatant::getHP() const { return currentHealth; }
int Combatant::getMP() const { return currentMagicPoints; }
int Combatant::getX() const { return xPos; }
int Combatant::getY() const { return yPos; }
int Combatant::getInitiative() const { return initiative; }
int Combatant::getMorale() const { return morale; }
bool Combatant::isAlive() const { return currentHealth > 0 && !fled; }
bool Combatant::isGuarding() const { return guarding; }
bool Combatant::hasFled() const { return fled; }
bool Combatant::isBroken() const { return morale < 0; }

const Armor& Combatant::getArmor() const { return equippedArmor; }

int Combatant::getEffectiveDR() const {
    int dr = equippedArmor.damageResistance;
    for (const auto& s : statuses) {
        if (s.name == "Acid") {
            dr -= s.potency;
        }
    }
    if (dr < -80) dr = -80;
    return dr;
}

const std::vector<Item>& Combatant::getInventory() const { return inventory; }
const std::vector<Spell>& Combatant::getSpells() const { return knownSpells; }
const Weapon& Combatant::getWeapon() const { return equippedWeapon; }

void Combatant::setPosition(int x, int y) { xPos = x; yPos = y; }
void Combatant::equipArmor(const Armor& armor) { equippedArmor = armor; }
void Combatant::equipWeapon(const Weapon& weapon) { equippedWeapon = weapon; }
void Combatant::learnSpell(const Spell& spell) { knownSpells.push_back(spell); }

void Combatant::addItem(const Item& item) {
    for (auto& i : inventory) {
        if (i.name == item.name) {
            i.quantity += item.quantity;
            return;
        }
    }
    inventory.push_back(item);
}

void Combatant::startTurn() {
    if (guarding) {
        std::cout << " >> " << name << " drops their guard.\n";
        guarding = false;
    }
}

void Combatant::addTicks(int ticks) {
    initiative += ticks;
    for (int t = 0; t < ticks; ++t) {
        for (auto it = statuses.begin(); it != statuses.end(); ) {
            if (it->name == "Burn") {
                currentHealth -= it->potency;
                if (currentHealth < 0) currentHealth = 0;
            }
            it->durationTicks--;
            if (it->durationTicks <= 0) {
                if (it->name == "Burn") std::cout << " >> " << name << "'s burns fade.\n";
                if (it->name == "Acid") std::cout << " >> Acid drips off " << name << "'s armor.\n";
                it = statuses.erase(it);
            }
            else {
                ++it;
            }
        }
        if (currentHealth <= 0) break;
    }
    if (currentHealth <= 0 && !fled) {
        std::cout << " >> " << name << " succumbed to damage!\n";
    }
}

void Combatant::guard() {
    guarding = true;
    std::cout << " >> " << name << " enters Guard Stance! (+100 DR)\n";
}

void Combatant::flee() {
    fled = true;
    xPos = -1;
    yPos = -1;
}

void Combatant::reduceMorale(int amount) {
    morale -= amount;
    if (morale < 0) {
        std::cout << " >> " << name << " is MENTALLY BROKEN! (Morale: " << morale << ")\n";
    }
}

void Combatant::regainMorale(int amount) {
    morale += amount;
    std::cout << " >> " << name << " regains " << amount << " Morale. (Current: " << morale << ")\n";
}

void Combatant::drainMP(int amount) {
    currentMagicPoints -= amount;
    if (currentMagicPoints < 0) currentMagicPoints = 0;
    std::cout << " >> " << name << " loses " << amount << " MP! (MP: " << currentMagicPoints << ")\n";
}

void Combatant::restoreMP(int amount) {
    currentMagicPoints += amount;
    if (currentMagicPoints > maxMagicPoints) currentMagicPoints = maxMagicPoints;
    std::cout << " >> " << name << " restores " << amount << " MP! (MP: " << currentMagicPoints << "/" << maxMagicPoints << ")\n";
}

void Combatant::applyStatus(std::string name, int duration, int potency) {
    StatusEffect effect;
    effect.name = name;
    effect.durationTicks = duration;
    effect.potency = potency;
    statuses.push_back(effect);
    std::cout << " >> " << this->name << " is affected by " << name << "! (" << duration << " ticks)\n";
}

void Combatant::takeDamage(int amount, std::string element, Grid* grid) {
    currentHealth -= amount;
    if (currentHealth < 0) currentHealth = 0;
    std::cout << " >> " << name << " takes " << amount << " damage! (HP: " << currentHealth << "/" << maxHealth << ")\n";

    if (element == "Psi") {
        std::cout << " >> Psi attack strikes the mind!\n";
        reduceMorale(amount);
    }
    if (element == "Electricity") {
        drainMP(amount / 2);
    }

    if (equippedArmor.elementType == "Poison" && grid != nullptr && xPos != -1) {
        int reflectDmg = amount / 2;
        if (reflectDmg > 0) {
            std::cout << " >> Poison Armor spews toxins! Reflecting " << reflectDmg << " damage!\n";
            int checkX[] = { 0, 0, 1, -1 };
            int checkY[] = { 1, -1, 0, 0 };
            for (int i = 0; i < 4; ++i) {
                Combatant* neighbor = grid->getCombatantAt(xPos + checkX[i], yPos + checkY[i]);
                if (neighbor && neighbor->isAlive() && neighbor->getTeam() != team) {
                    neighbor->takeDamage(reflectDmg, "Reflect", grid);
                }
            }
        }
    }

    if (currentHealth == 0) {
        std::cout << " >> " << name << " has been defeated!\n";
    }
}

void Combatant::heal(int amount) {
    currentHealth += amount;
    if (currentHealth > maxHealth) currentHealth = maxHealth;
    std::cout << " >> " << name << " recovers " << amount << " HP! (HP: " << currentHealth << "/" << maxHealth << ")\n";
}

void Combatant::printStats() const {
    std::cout << "Name: " << name << " | HP: " << currentHealth << "/" << maxHealth
        << " | MP: " << currentMagicPoints << "/" << maxMagicPoints
        << " | Init: " << initiative
        << " | Morale: " << morale
        << (isBroken() ? " [BROKEN]" : "")
        << (guarding ? " [GUARDING]" : "")
        << " | Wpn: " << equippedWeapon.name
        << " | Armor: " << equippedArmor.name << "\n";
}

bool Combatant::checkRange(const Combatant& target, float range) const {
    if (xPos == -1 || target.getX() == -1) return true;
	float dist = getDistance(xPos, yPos, target.getX(), target.getY());
    if (dist > range) {
        return false;
    }
    return true;
}

// ------------------------------------------
// CORE COMBAT LOGIC
// ------------------------------------------

bool Combatant::attack(Combatant& target, Grid& grid) {
    if (!checkRange(target, equippedWeapon.range)) {
        std::cout << " >> Target out of range for attack!\n";
        return false;
    }

    std::cout << name << " attacks " << target.getName() << " with " << equippedWeapon.name << " (" << equippedWeapon.elementType << ")!\n";

    for (int i = 0; i < equippedWeapon.numberOfAttacks; ++i) {
        if (!target.isAlive()) break;

        float hitChance = equippedWeapon.accuracy - target.getArmor().evasion;
        float roll = getRandomFloat(0.0f, 1.0f);

        if (roll > hitChance) {
            std::cout << " - Attack " << (i + 1) << " MISSED!\n";
            continue;
        }

        int multiplier = getRandomInt(7, 13);
        int productDamage = (equippedWeapon.physicalAttack * multiplier) / 10;
        int critDamage = 0;
		int finalDamage = 0;

        bool isCrit = getRandomFloat(0.0f, 1.0f) <= 0.05f;

        if (isCrit) {
            std::cout << " - CRITICAL HIT! ";
            critDamage = productDamage;
        }
        else {
            int damageAfterThreshold = productDamage - target.getArmor().damageThreshold;
            if (damageAfterThreshold < 1) damageAfterThreshold = 1;

            int baseDR = target.getEffectiveDR();
            if (target.isGuarding()) baseDR += 100;
            if (baseDR < -80) baseDR = -80;

            finalDamage = (damageAfterThreshold * 100) / (100 + baseDR);
        }

        float elemMult = getElementalMultiplier(equippedWeapon.elementType, target.getArmor().elementType);
        finalDamage = static_cast<int>(finalDamage * elemMult);

        if (elemMult > 1.0f) std::cout << "(Weakness Hit!) ";
        if (elemMult < 1.0f) std::cout << "(Resisted) ";

        std::string elem = equippedWeapon.elementType;

        if (elem == "Fire") {
            int burnDmg = productDamage / 20;
            target.applyStatus("Burn", 5, burnDmg);
        }
        else if (elem == "Acid") {
            int acidPotency = finalDamage / 3;
            target.applyStatus("Acid", 7, acidPotency);
        }
        else if (elem == "Ice") {
            int ticksToAdd = static_cast<int>(std::ceil(finalDamage * 0.01));
            std::cout << " >> Ice chills " << target.getName() << "! (+" << ticksToAdd << " Init Ticks)\n";
            target.addTicks(ticksToAdd);
        }
        else if (elem == "Bio") {
            int healAmt = finalDamage / 2;
            std::cout << " >> Bio-leech absorbs health!\n";
            this->heal(healAmt);
        }
		int damageTaken = finalDamage + critDamage;
        target.takeDamage(damageTaken, elem, &grid);
    }
    return true;
}

bool Combatant::castSpell(Combatant& primaryTarget, int spellIndex, Grid& grid) {
    if (spellIndex < 0 || spellIndex >= knownSpells.size()) {
        std::cout << "Invalid spell selection.\n";
        return false;
    }
    const Spell& spell = knownSpells[spellIndex];

    if (currentMagicPoints < spell.mpCost) {
        std::cout << " >> Not enough MP! (Cost: " << spell.mpCost << ", Have: " << currentMagicPoints << ")\n";
        return false;
    }

    // Range Check to Center Target
    if (!checkRange(primaryTarget, spell.range)) {
        std::cout << " >> Target out of range for spell!\n";
        return false;
    }

    currentMagicPoints -= spell.mpCost;
    std::cout << name << " casts " << spell.name << " (" << spell.elementType << ")!\n";

    // Define Spell Effect Application Lambda
    auto applySpellEffect = [&](Combatant* victim) {
        int magicDef = victim->getArmor().magicalDefense;
        if (magicDef < -80) magicDef = -80;

        int damage = (spell.magicalAttack * 100) / (100 + magicDef);

        float elemMult = getElementalMultiplier(spell.elementType, victim->getArmor().elementType);
        damage = static_cast<int>(damage * elemMult);

        // Log individual hit
        std::cout << "  -> Hit " << victim->getName() << ": ";
        if (elemMult > 1.0f) std::cout << "(Weakness) ";
        if (elemMult < 1.0f) std::cout << "(Resisted) ";

        std::string elem = spell.elementType;

        if (elem == "Fire") {
            int burnDmg = spell.magicalAttack / 20;
            victim->applyStatus("Burn", 5, burnDmg);
        }
        else if (elem == "Acid") {
            int acidPotency = damage / 3;
            victim->applyStatus("Acid", 7, acidPotency);
        }
        else if (elem == "Ice") {
            int ticksToAdd = static_cast<int>(std::ceil(damage * 0.01));
            std::cout << "Ice chills! (+" << ticksToAdd << " Init Ticks) ";
            victim->addTicks(ticksToAdd);
        }
        else if (elem == "Bio") {
            int healAmt = damage / 2;
            std::cout << "Bio-leech! ";
            this->heal(healAmt);
        }

        victim->takeDamage(damage, elem, &grid);
        };

    // AOE Logic
    if (primaryTarget.getX() == -1) return false;

    // Scan the grid for all valid targets in AOE
    for (int y = 0; y < grid.getHeight(); ++y) {
        for (int x = 0; x < grid.getWidth(); ++x) {
            Combatant* potential = grid.getCombatantAt(x, y);
            if (potential && potential->isAlive()) {
                // Distance to Primary Target
				float dist = getDistance(primaryTarget.getX(), primaryTarget.getY(), x, y);

                // Check if within AOE radius
                if (dist <= spell.aoe) {
                    // Team Filter
                    bool isAlly = (potential->getTeam() == this->team);

                    if (spell.category == "Buff") {
                        // Buffs only hit same team
                        if (isAlly) applySpellEffect(potential);
                    }
                    else { // Debuff / Attack
                        // Debuffs only hit different team
                        if (!isAlly) applySpellEffect(potential);
                    }
                }
            }
        }
    }

    return true;
}

bool Combatant::useItem(Combatant& target, int itemIndex) {
    if (itemIndex < 0 || itemIndex >= inventory.size()) return false;

    Item& item = inventory[itemIndex];
    if (item.quantity <= 0) {
        std::cout << " >> Not enough items!\n";
        return false;
    }

    if (!checkRange(target, item.range)) {
        std::cout << " >> Target out of range for item!\n";
        return false;
    }

    item.quantity--;
    std::cout << name << " uses " << item.name << " on " << target.getName() << "!\n";

    if (item.category == "Healing") {
        target.heal(item.potency);
    }
    else if (item.category == "RestoreMP") {
        target.restoreMP(item.potency);
    }
    else if (item.category == "Buff") {
        std::cout << " >> " << target.getName() << " is Buffed!\n";
    }
    else if (item.category == "Debuff") {
        std::cout << " >> " << target.getName() << " is Debuffed!\n";
    }
    return true;
}


// ==========================================
// Battle Manager Implementation
// ==========================================

void BattleManager::addParticipant(Combatant* c) {
    participants.push_back(c);
}

const std::vector<Combatant*>& BattleManager::getParticipants() const {
    return participants;
}

Combatant* BattleManager::getNextActiveCombatant() {
    int minInit = std::numeric_limits<int>::max();
    std::vector<Combatant*> tiedCombatants;

    for (auto c : participants) {
        if (!c->isAlive()) continue;

        int init = c->getInitiative();
        if (init < minInit) {
            minInit = init;
            tiedCombatants.clear();
            tiedCombatants.push_back(c);
        }
        else if (init == minInit) {
            tiedCombatants.push_back(c);
        }
    }

    if (tiedCombatants.empty()) return nullptr;

    if (tiedCombatants.size() == 1) {
        return tiedCombatants[0];
    }
    else {
        int idx = getRandomInt(0, tiedCombatants.size() - 1);
        std::cout << "[Info] Tie detected for Initiative " << minInit << ". Randomly resolving...\n";
        return tiedCombatants[idx];
    }
}

std::string BattleManager::getWinner() {
    bool goodAlive = false;
    bool badAlive = false;

    for (auto c : participants) {
        if (c->isAlive()) {
            if (c->getTeam() == "Good Guys") goodAlive = true;
            if (c->getTeam() == "Bad Guys") badAlive = true;
        }
    }

    if (!goodAlive && !badAlive) return "Draw";
    if (!goodAlive) return "Bad Guys";
    if (!badAlive) return "Good Guys";

    return "None";
}

// ==========================================
// Grid Implementation
// ==========================================
Grid::Grid(int w, int h) : width(w), height(h) {
    terrainMap.resize(h, std::vector<int>(w, 0));
    combatantMap.resize(h, std::vector<Combatant*>(w, nullptr));
}

Combatant* Grid::getCombatantAt(int x, int y) {
    if (x < 0 || x >= width || y < 0 || y >= height) return nullptr;
    return combatantMap[y][x];
}

bool Grid::placeCombatant(Combatant* c, int x, int y) {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    if (combatantMap[y][x] != nullptr) return false;

    int oldX = c->getX();
    int oldY = c->getY();
    if (oldX != -1 && oldY != -1) {
        combatantMap[oldY][oldX] = nullptr;
    }

    combatantMap[y][x] = c;
    c->setPosition(x, y);
    return true;
}

int Grid::moveCombatant(Combatant* c, int dx, int dy) {
    int curX = c->getX();
    int curY = c->getY();
    if (curX == -1) return 0;

    // 1. Check Adjacency Rule
    bool isEngaged = false;
    int checkX[] = { 0, 0, 1, -1 };
    int checkY[] = { 1, -1, 0, 0 };

    for (int i = 0; i < 4; ++i) {
        int nx = curX + checkX[i];
        int ny = curY + checkY[i];

        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            if (combatantMap[ny][nx] != nullptr) {
                if (combatantMap[ny][nx]->getTeam() != c->getTeam() && combatantMap[ny][nx]->isAlive()) {
                    isEngaged = true;
                    break;
                }
            }
        }
    }

    int tickCost = isEngaged ? (COST_MOVE_BASE + COST_MOVE_PENALTY) : COST_MOVE_BASE;

    // 2. Calculate New Position
    int newX = curX + dx;
    int newY = curY + dy;

    // Flee check
    if (newX < 0 || newX >= width || newY < 0 || newY >= height) {
        std::cout << " >> " << c->getName() << " runs off the battlefield!\n";
        c->flee();
        combatantMap[curY][curX] = nullptr;
        return tickCost;
    }

    // Occupied check
    if (combatantMap[newY][newX] != nullptr) {
        std::cout << "[Movement] Blocked (Occupied by " << combatantMap[newY][newX]->getName() << ")\n";
        return 0; // Failed
    }

    // 3. Execute Move
    combatantMap[curY][curX] = nullptr;
    combatantMap[newY][newX] = c;
    c->setPosition(newX, newY);
    std::cout << "[Movement] " << c->getName() << " moved to (" << newX << "," << newY << "). ";

    if (isEngaged) std::cout << "(Engaged move: +" << tickCost << " ticks)\n";
    else std::cout << "(Standard move: +" << tickCost << " ticks)\n";

    return tickCost;
}

void Grid::drawGrid() {
    std::cout << "\n--- Battlefield ---\n";
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            std::cout << "[";
            if (combatantMap[y][x] != nullptr) {
                if (combatantMap[y][x]->isAlive())
                    std::cout << combatantMap[y][x]->getName()[0];
                else
                    std::cout << "x";
            }
            else {
                std::cout << " ";
            }
            std::cout << "]";
        }
        std::cout << "\n";
    }
    std::cout << "-------------------\n";
}

// ==========================================
// Player Implementation
// ==========================================
Player::Player(std::string n) : name(n) {}

void Player::addCombatant(std::shared_ptr<Combatant> c) {
    party.push_back(c);
}

std::vector<std::shared_ptr<Combatant>>& Player::getParty() {
    return party;
}

std::string Player::getName() const { return name; }

void Player::listParty() const {
    std::cout << "Player " << name << "'s Party:\n";
    for (const auto& member : party) {
        std::cout << " - ";
        member->printStats();
    }
}