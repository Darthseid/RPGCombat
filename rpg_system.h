#ifndef RPG_SYSTEM_H
#define RPG_SYSTEM_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>

class Combatant;
class Grid;

enum ActionCost {
    COST_MOVE_BASE = 1,
    COST_MOVE_PENALTY = 6,
    COST_GUARD = 2,
    COST_ITEM = 3,
    COST_SPELL = 4,
    COST_ATTACK = 6
};

// ==========================================
// Status Effect Struct
// ==========================================
struct StatusEffect {
    std::string name;
    int durationTicks;
    int potency;
};

// ==========================================
// 1. Item & Ability Classes
// ==========================================

class Item {
public:
    std::string name;
    int quantity;
    float range;
    std::string category;
    int potency;

    Item(std::string n, int qty, float rng, std::string cat, int pot);
};

class Armor {
public:
    std::string name;
    int damageResistance;
    int damageThreshold;
    float evasion;
    std::string elementType;
    int magicalDefense;

    Armor(std::string n, int dr, int dt, float eva, std::string elem, int magDef);
    Armor();
};

class Weapon {
public:
    std::string name;
    int physicalAttack;
    float accuracy;
    float range;
    int numberOfAttacks;
    std::string elementType;

    Weapon(std::string n, int atk, float acc, float rng, int num, std::string elem);
    Weapon();
};

class Spell {
public:
    std::string name;
    int magicalAttack;
    int mpCost;
    float range;
    int duration;
    std::string elementType;
    int aoe;               // New: Area of Effect
    std::string category;  // New: "Buff" or "Debuff"

    Spell(std::string n, int matk, int cost, float rng, int dur, std::string elem, int area, std::string cat);
};

// ==========================================
// 2. Combatant Class
// ==========================================

class Combatant {
private:
    std::string name;
    std::string team;
    int maxHealth;
    int currentHealth;
    int maxMagicPoints;
    int currentMagicPoints;

    int initiative;
    int morale;
    bool guarding = false;
    bool fled = false;

    Armor equippedArmor;
    Weapon equippedWeapon;
    std::vector<Spell> knownSpells;
    std::vector<Item> inventory;
    std::vector<StatusEffect> statuses;

    int xPos = -1;
    int yPos = -1;

public:
    Combatant(std::string n, std::string teamName, int hp, int mp, int init, int mor);

    // Getters
    std::string getName() const;
    std::string getTeam() const;
    int getHP() const;
    int getMP() const;
    int getX() const;
    int getY() const;
    int getInitiative() const;
    bool isAlive() const;
    bool isGuarding() const;
    bool hasFled() const;
    bool isBroken() const;
    int getMorale() const;

    const Armor& getArmor() const;
    int getEffectiveDR() const;
    const std::vector<Item>& getInventory() const;
    const std::vector<Spell>& getSpells() const;
    const Weapon& getWeapon() const;

    // Actions
    void setPosition(int x, int y);
    void equipArmor(const Armor& armor);
    void equipWeapon(const Weapon& weapon);
    void learnSpell(const Spell& spell);
    void addItem(const Item& item);
    void printStats() const;

    // Status Changes
    void takeDamage(int amount, std::string element = "None", Grid* grid = nullptr);
    void drainMP(int amount);
    void heal(int amount);
    void restoreMP(int amount);
    void flee();
    void reduceMorale(int amount);
    void regainMorale(int amount);
    void applyStatus(std::string name, int duration, int potency);

    // Turn Management
    void startTurn();
    void addTicks(int ticks);
    void guard();

    // Helper
    bool checkRange(const Combatant& target, float range) const;

    // Combat Functions 
    bool attack(Combatant& target, Grid& grid);
    bool castSpell(Combatant& target, int spellIndex, Grid& grid);
    bool useItem(Combatant& target, int itemIndex);
};

// ==========================================
// 3. Battle Manager
// ==========================================
class BattleManager {
private:
    std::vector<Combatant*> participants;

public:
    void addParticipant(Combatant* c);
    Combatant* getNextActiveCombatant();
    std::string getWinner();
    const std::vector<Combatant*>& getParticipants() const;
};

// ==========================================
// 4. Grid Class
// ==========================================
class Grid {
private:
    int width;
    int height;
    std::vector<std::vector<int>> terrainMap;
    std::vector<std::vector<Combatant*>> combatantMap;

public:
    Grid(int w, int h);
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    Combatant* getCombatantAt(int x, int y);

    bool placeCombatant(Combatant* c, int x, int y);
    int moveCombatant(Combatant* c, int dx, int dy);
    void drawGrid();
};

// ==========================================
// 5. Player Class
// ==========================================
class Player {
private:
    std::string name;
    std::vector<std::shared_ptr<Combatant>> party;

public:
    Player(std::string n);
    void addCombatant(std::shared_ptr<Combatant> c);
    std::vector<std::shared_ptr<Combatant>>& getParty();
    std::string getName() const;
    void listParty() const;
};

// Helper
float getElementalMultiplier(const std::string& atkElem, const std::string& defElem);
float getDistance(int x1, int y1, int x2, int y2);

#endif