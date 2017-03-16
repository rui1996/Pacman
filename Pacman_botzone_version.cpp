#include <fstream>
#include <list>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <string>
#include <cstring>
#include <stack>
#include <cmath>
#include <stdexcept>
#include "jsoncpp/json.h"
 
#define FIELD_MAX_HEIGHT 20
#define FIELD_MAX_WIDTH 20
#define MAX_GENERATOR_COUNT 4 // ÿ������1
#define MAX_PLAYER_COUNT 4
#define MAX_TURN 100
#define pb push_back
// ��Ҳ����ѡ�� using namespace std; ���ǻ���Ⱦ�����ռ�
using std::string;
using std::swap;
using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::runtime_error;
 
// ƽ̨�ṩ�ĳԶ�������߼��������
const int INF = 10000000;
namespace Pacman
{
    const time_t seed = time(0);
    const int dx[] = { 0, 1, 0, -1, 1, 1, -1, -1 }, dy[] = { -1, 0, 1, 0, -1, 1, 1, -1 };
 
    // ö�ٶ��壻ʹ��ö����Ȼ���˷ѿռ䣨sizeof(GridContentType) == 4�������Ǽ��������32λ������Ч�ʸ���
 
    // ÿ�����ӿ��ܱ仯�����ݣ�����á����߼��������
    enum GridContentType
    {
        empty = 0, // ��ʵ�����õ�
        player1 = 1, // 1�����
        player2 = 2, // 2�����
        player3 = 4, // 3�����
        player4 = 8, // 4�����
        playerMask = 1 | 2 | 4 | 8, // ���ڼ����û����ҵ�
        smallFruit = 16, // С����
        largeFruit = 32 // ����
    };
 
    // �����ID��ȡ��������ҵĶ�����λ
    GridContentType playerID2Mask[] = { player1, player2, player3, player4 };
    string playerID2str[] = { "0", "1", "2", "3" };
 
    // ��ö��Ҳ��������Щ�����ˣ����ӻ�������
    template<typename T>
    inline T operator |=(T &a, const T &b)
    {
        return a = static_cast<T>(static_cast<int>(a) | static_cast<int>(b));
    }
    template<typename T>
    inline T operator |(const T &a, const T &b)
    {
        return static_cast<T>(static_cast<int>(a) | static_cast<int>(b));
    }
    template<typename T>
    inline T operator &=(T &a, const T &b)
    {
        return a = static_cast<T>(static_cast<int>(a) & static_cast<int>(b));
    }
    template<typename T>
    inline T operator &(const T &a, const T &b)
    {
        return static_cast<T>(static_cast<int>(a) & static_cast<int>(b));
    }
    template<typename T>
    inline T operator ++(T &a)
    {
        return a = static_cast<T>(static_cast<int>(a) + 1);
    }
    template<typename T>
    inline T operator ~(const T &a)
    {
        return static_cast<T>(~static_cast<int>(a));
    }
 
    // ÿ�����ӹ̶��Ķ���������á����߼��������
    enum GridStaticType
    {
        emptyWall = 0, // ��ʵ�����õ�
        wallNorth = 1, // ��ǽ����������ٵķ���
        wallEast = 2, // ��ǽ�����������ӵķ���
        wallSouth = 4, // ��ǽ�����������ӵķ���
        wallWest = 8, // ��ǽ����������ٵķ���
        generator = 16 // ���Ӳ�����
    };
 
    // ���ƶ�����ȡ����������赲�ŵ�ǽ�Ķ�����λ
    GridStaticType direction2OpposingWall[] = { wallNorth, wallEast, wallSouth, wallWest };
 
    // ���򣬿��Դ���dx��dy���飬ͬʱҲ������Ϊ��ҵĶ���
    enum Direction
    {
        stay = -1,
        up = 0,
        right = 1,
        down = 2,
        left = 3,
        // ������⼸��ֻ��Ϊ�˲��������򷽱㣬����ʵ���õ�
        ur = 4, // ����
        dr = 5, // ����
        dl = 6, // ����
        ul = 7 // ����
    };
 
    // �����ϴ�����������
    struct FieldProp
    {
        int row, col;
    };
 
    // �����ϵ����
    struct Player : FieldProp
    {
        int strength;
        int powerUpLeft;
        bool dead;
    };
 
    // �غ��²����Ķ��ӵ�����
    struct NewFruits
    {
        FieldProp newFruits[MAX_GENERATOR_COUNT * 8];
        int newFruitCount;
    } newFruits[MAX_TURN];
    int newFruitsCount = 0;
 
    // ״̬ת�Ƽ�¼�ṹ
    struct TurnStateTransfer
    {
        enum StatusChange // �����
        {
            none = 0,
            ateSmall = 1,
            ateLarge = 2,
            powerUpCancel = 4,
            die = 8,
            error = 16
        };
 
        // ���ѡ���Ķ���
        Direction actions[MAX_PLAYER_COUNT];
 
        // �˻غϸ���ҵ�״̬�仯
        StatusChange change[MAX_PLAYER_COUNT];
 
        // �˻غϸ���ҵ������仯
        int strengthDelta[MAX_PLAYER_COUNT];
    };
 
    // ��Ϸ��Ҫ�߼������࣬��������������غ����㡢״̬ת�ƣ�ȫ��Ψһ
    class GameField
    {
    private:
        // Ϊ�˷��㣬��������Զ�����private��
 
        // ��¼ÿ�غϵı仯��ջ��
        TurnStateTransfer backtrack[MAX_TURN];
 
        // ��������Ƿ��Ѿ�����
        static bool constructed;
 
    public:
        // ���صĳ��Ϳ�
        int height, width;
        int generatorCount;
        int GENERATOR_INTERVAL, LARGE_FRUIT_DURATION, LARGE_FRUIT_ENHANCEMENT;
 
        // ���ظ��ӹ̶�������
        GridStaticType fieldStatic[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
 
        // ���ظ��ӻ�仯������
        GridContentType fieldContent[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        int generatorTurnLeft; // ���ٻغϺ��������
        int aliveCount; // �ж�����Ҵ��
        int smallFruitCount;
        int turnID;
        FieldProp generators[MAX_GENERATOR_COUNT]; // ����Щ���Ӳ�����
        Player players[MAX_PLAYER_COUNT]; // ����Щ���
 
        // ���ѡ���Ķ���
        Direction actions[MAX_PLAYER_COUNT];
 
        // �ָ����ϴγ���״̬������һ·�ָ����ʼ��
        // �ָ�ʧ�ܣ�û��״̬�ɻָ�������false
        bool PopState()
        {
            if (turnID <= 0)
                return false;
 
            const TurnStateTransfer &bt = backtrack[--turnID];
            int i, _;
 
            // �������ָ�״̬
 
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                GridContentType &content = fieldContent[_p.row][_p.col];
                TurnStateTransfer::StatusChange change = bt.change[_];
 
                if (!_p.dead)
                {
                    // 5. �󶹻غϻָ�
                    if (_p.powerUpLeft || change & TurnStateTransfer::powerUpCancel)
                        _p.powerUpLeft++;
 
                    // 4. �³�����
                    if (change & TurnStateTransfer::ateSmall)
                    {
                        content |= smallFruit;
                        smallFruitCount++;
                    }
                    else if (change & TurnStateTransfer::ateLarge)
                    {
                        content |= largeFruit;
                        _p.powerUpLeft -= LARGE_FRUIT_DURATION;
                    }
                }
 
                // 2. �������
                if (change & TurnStateTransfer::die)
                {
                    _p.dead = false;
                    aliveCount++;
                    content |= playerID2Mask[_];
                }
 
                // 1. ���λ�Ӱ
                if (!_p.dead && bt.actions[_] != stay)
                {
                    fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
                    _p.row = (_p.row - dy[bt.actions[_]] + height) % height;
                    _p.col = (_p.col - dx[bt.actions[_]] + width) % width;
                    fieldContent[_p.row][_p.col] |= playerID2Mask[_];
                }
 
                // 0. ���겻�Ϸ������
                if (change & TurnStateTransfer::error)
                {
                    _p.dead = false;
                    aliveCount++;
                    content |= playerID2Mask[_];
                }
 
                // *. �ָ�����
                if (!_p.dead)
                    _p.strength -= bt.strengthDelta[_];
            }
 
            // 3. �ջض���
            if (generatorTurnLeft == GENERATOR_INTERVAL)
            {
                generatorTurnLeft = 1;
                NewFruits &fruits = newFruits[--newFruitsCount];
                for (i = 0; i < fruits.newFruitCount; i++)
                {
                    fieldContent[fruits.newFruits[i].row][fruits.newFruits[i].col] &= ~smallFruit;
                    smallFruitCount--;
                }
            }
            else
                generatorTurnLeft++;
 
            return true;
        }
 
        // �ж�ָ�������ָ�������ƶ��ǲ��ǺϷ��ģ�û��ײǽ��
        inline bool ActionValid(int playerID, Direction &dir) const
        {
            if (dir == stay)
                return true;
            const Player &p = players[playerID];
            const GridStaticType &s = fieldStatic[p.row][p.col];
            return dir >= -1 && dir < 4 && !(s & direction2OpposingWall[dir]);
        }
 
        // ����actionsд����Ҷ�����������һ�غϾ��棬����¼֮ǰ���еĳ���״̬���ɹ��պ�ָ���
        // ���վֵĻ��ͷ���false
        bool NextTurn()
        {
            int _, i, j;
 
            TurnStateTransfer &bt = backtrack[turnID];
            memset(&bt, 0, sizeof(bt));
 
            // 0. ɱ�����Ϸ�����
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &p = players[_];
                if (!p.dead)
                {
                    Direction &action = actions[_];
                    if (action == stay)
                        continue;
 
                    if (!ActionValid(_, action))
                    {
                        bt.strengthDelta[_] += -p.strength;
                        bt.change[_] = TurnStateTransfer::error;
                        fieldContent[p.row][p.col] &= ~playerID2Mask[_];
                        p.strength = 0;
                        p.dead = true;
                        aliveCount--;
                    }
                    else
                    {
                        // �������Լ�ǿ��׳������ǲ���ǰ����
                        GridContentType target = fieldContent
                            [(p.row + dy[action] + height) % height]
                            [(p.col + dx[action] + width) % width];
                        if (target & playerMask)
                            for (i = 0; i < MAX_PLAYER_COUNT; i++)
                                if (target & playerID2Mask[i] && players[i].strength > p.strength)
                                    action = stay;
                    }
                }
            }
 
            // 1. λ�ñ仯
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead)
                    continue;
 
                bt.actions[_] = actions[_];
 
                if (actions[_] == stay)
                    continue;
 
                // �ƶ�
                fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
                _p.row = (_p.row + dy[actions[_]] + height) % height;
                _p.col = (_p.col + dx[actions[_]] + width) % width;
                fieldContent[_p.row][_p.col] |= playerID2Mask[_];
            }
 
            // 2. ��һ�Ź
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead)
                    continue;
 
                // �ж��Ƿ��������һ��
                int player, containedCount = 0;
                int containedPlayers[MAX_PLAYER_COUNT];
                for (player = 0; player < MAX_PLAYER_COUNT; player++)
                    if (fieldContent[_p.row][_p.col] & playerID2Mask[player])
                        containedPlayers[containedCount++] = player;
 
                if (containedCount > 1)
                {
                    // NAIVE
                    for (i = 0; i < containedCount; i++)
                        for (j = 0; j < containedCount - i - 1; j++)
                            if (players[containedPlayers[j]].strength < players[containedPlayers[j + 1]].strength)
                                swap(containedPlayers[j], containedPlayers[j + 1]);
 
                    int begin;
                    for (begin = 1; begin < containedCount; begin++)
                        if (players[containedPlayers[begin - 1]].strength > players[containedPlayers[begin]].strength)
                            break;
 
                    // ��Щ��ҽ��ᱻɱ��
                    int lootedStrength = 0;
                    for (i = begin; i < containedCount; i++)
                    {
                        int id = containedPlayers[i];
                        Player &p = players[id];
 
                        // �Ӹ���������
                        fieldContent[p.row][p.col] &= ~playerID2Mask[id];
                        p.dead = true;
                        int drop = p.strength / 2;
                        bt.strengthDelta[id] += -drop;
                        bt.change[id] |= TurnStateTransfer::die;
                        lootedStrength += drop;
                        p.strength -= drop;
                        aliveCount--;
                    }
 
                    // ������������
                    int inc = lootedStrength / begin;
                    for (i = 0; i < begin; i++)
                    {
                        int id = containedPlayers[i];
                        Player &p = players[id];
                        bt.strengthDelta[id] += inc;
                        p.strength += inc;
                    }
                }
            }
 
            // 3. ��������
            if (--generatorTurnLeft == 0)
            {
                generatorTurnLeft = GENERATOR_INTERVAL;
                NewFruits &fruits = newFruits[newFruitsCount++];
                fruits.newFruitCount = 0;
                for (i = 0; i < generatorCount; i++)
                    for (Direction d = up; d < 8; ++d)
                    {
                        // ȡ�࣬�������ر߽�
                        int r = (generators[i].row + dy[d] + height) % height, c = (generators[i].col + dx[d] + width) % width;
                        if (fieldStatic[r][c] & generator || fieldContent[r][c] & (smallFruit | largeFruit))
                            continue;
                        fieldContent[r][c] |= smallFruit;
                        fruits.newFruits[fruits.newFruitCount].row = r;
                        fruits.newFruits[fruits.newFruitCount++].col = c;
                        smallFruitCount++;
                    }
            }
 
            // 4. �Ե�����
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead)
                    continue;
 
                GridContentType &content = fieldContent[_p.row][_p.col];
 
                // ֻ���ڸ�����ֻ���Լ���ʱ����ܳԵ�����
                if (content & playerMask & ~playerID2Mask[_])
                    continue;
 
                if (content & smallFruit)
                {
                    content &= ~smallFruit;
                    _p.strength++;
                    bt.strengthDelta[_]++;
                    smallFruitCount--;
                    bt.change[_] |= TurnStateTransfer::ateSmall;
                }
                else if (content & largeFruit)
                {
                    content &= ~largeFruit;
                    if (_p.powerUpLeft == 0)
                    {
                        _p.strength += LARGE_FRUIT_ENHANCEMENT;
                        bt.strengthDelta[_] += LARGE_FRUIT_ENHANCEMENT;
                    }
                    _p.powerUpLeft += LARGE_FRUIT_DURATION;
                    bt.change[_] |= TurnStateTransfer::ateLarge;
                }
            }
 
            // 5. �󶹻غϼ���
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead)
                    continue;
 
                if (_p.powerUpLeft > 0 && --_p.powerUpLeft == 0)
                {
                    _p.strength -= LARGE_FRUIT_ENHANCEMENT;
                    bt.change[_] |= TurnStateTransfer::powerUpCancel;
                    bt.strengthDelta[_] += -LARGE_FRUIT_ENHANCEMENT;
                }
            }
 
            ++turnID;
 
            // �Ƿ�ֻʣһ�ˣ�
            if (aliveCount <= 1)
            {
                for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
                    if (!players[_].dead)
                    {
                        bt.strengthDelta[_] += smallFruitCount;
                        players[_].strength += smallFruitCount;
                    }
                return false;
            }
 
            // �Ƿ�غϳ��ޣ�
            if (turnID >= 100)
                return false;
 
            return true;
        }
 
        // ��ȡ�������������룬���ص��Ի��ύƽ̨ʹ�ö����ԡ�
        // ����ڱ��ص��ԣ�����������Ŷ�ȡ������ָ�����ļ���Ϊ�����ļ���ʧ�ܺ���ѡ��ȴ��û�ֱ�����롣
        // ���ص���ʱ���Խ��ܶ����Ա������Windows�¿����� Ctrl-Z ��һ��������+�س�����ʾ���������������������ֻ����ܵ��м��ɡ�
        // localFileName ����ΪNULL
        // obtainedData ������Լ��ϻغϴ洢�����غ�ʹ�õ�����
        // obtainedGlobalData ������Լ��� Bot ����ǰ�洢������
        // ����ֵ���Լ��� playerID
        int ReadInput(const char *localFileName, string &obtainedData, string &obtainedGlobalData)
        {
            string str, chunk;
#ifdef _BOTZONE_ONLINE
            std::ios::sync_with_stdio(false); //��\\)
            getline(cin, str);
#else
            if (localFileName)
            {
                std::ifstream fin(localFileName);
                if (fin)
                    while (getline(fin, chunk) && chunk != "")
                        str += chunk;
                else
                    while (getline(cin, chunk) && chunk != "")
                        str += chunk;
            }
            else
                while (getline(cin, chunk) && chunk != "")
                    str += chunk;
#endif
            Json::Reader reader;
            Json::Value input;
            reader.parse(str, input);
 
            int len = input["requests"].size();
 
            // ��ȡ���ؾ�̬״��
            Json::Value field = input["requests"][(Json::Value::UInt) 0],
                staticField = field["static"], // ǽ��Ͳ�����
                contentField = field["content"]; // ���Ӻ����
            height = field["height"].asInt();
            width = field["width"].asInt();
            LARGE_FRUIT_DURATION = field["LARGE_FRUIT_DURATION"].asInt();
            LARGE_FRUIT_ENHANCEMENT = field["LARGE_FRUIT_ENHANCEMENT"].asInt();
            generatorTurnLeft = GENERATOR_INTERVAL = field["GENERATOR_INTERVAL"].asInt();
 
            PrepareInitialField(staticField, contentField);
 
            // ������ʷ�ָ�����
            for (int i = 1; i < len; i++)
            {
                Json::Value req = input["requests"][i];
                for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
                    if (!players[_].dead)
                        actions[_] = (Direction)req[playerID2str[_]]["action"].asInt();
                NextTurn();
            }
 
            obtainedData = input["data"].asString();
            obtainedGlobalData = input["globaldata"].asString();
 
            return field["id"].asInt();
        }
 
        // ���� static �� content ����׼�����صĳ�ʼ״��
        void PrepareInitialField(const Json::Value &staticField, const Json::Value &contentField)
        {
            int r, c, gid = 0;
            generatorCount = 0;
            aliveCount = 0;
            smallFruitCount = 0;
            generatorTurnLeft = GENERATOR_INTERVAL;
            for (r = 0; r < height; r++)
                for (c = 0; c < width; c++)
                {
                    GridContentType &content = fieldContent[r][c] = (GridContentType)contentField[r][c].asInt();
                    GridStaticType &s = fieldStatic[r][c] = (GridStaticType)staticField[r][c].asInt();
                    if (s & generator)
                    {
                        generators[gid].row = r;
                        generators[gid++].col = c;
                        generatorCount++;
                    }
                    if (content & smallFruit)
                        smallFruitCount++;
                    for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
                        if (content & playerID2Mask[_])
                        {
                            Player &p = players[_];
                            p.col = c;
                            p.row = r;
                            p.powerUpLeft = 0;
                            p.strength = 1;
                            p.dead = false;
                            aliveCount++;
                        }
                }
        }
 
        // ��ɾ��ߣ���������
        // action ��ʾ���غϵ��ƶ�����stay Ϊ���ƶ�
        // tauntText ��ʾ��Ҫ��������������������ַ�����������ʾ����Ļ�ϲ������κ����ã����ձ�ʾ������
        // data ��ʾ�Լ���洢����һ�غ�ʹ�õ����ݣ����ձ�ʾɾ��
        // globalData ��ʾ�Լ���洢���Ժ�ʹ�õ����ݣ��滻����������ݿ��Կ�Ծ�ʹ�ã���һֱ������� Bot �ϣ����ձ�ʾɾ��
        void WriteOutput(Direction action, string tauntText = "", string data = "", string globalData = "") const
        {
            Json::Value ret;
            ret["response"]["action"] = action;
            ret["response"]["tauntText"] = tauntText;
            ret["data"] = data;
            ret["globaldata"] = globalData;
            ret["debug"] = (Json::Int)seed;
 
#ifdef _BOTZONE_ONLINE
            Json::FastWriter writer; // ��������Ļ����þ��С���
#else
            Json::StyledWriter writer; // ���ص��������ÿ� > <
#endif
            cout << writer.write(ret) << endl;
        }
 
        // ������ʾ��ǰ��Ϸ״̬�������á�
        // �ύ��ƽ̨��ᱻ�Ż�����
        inline void DebugPrint() const
        {
#ifndef _BOTZONE_ONLINE
            printf("�غϺš�%d�����������%d��| ͼ�� ������[G] �����[0/1/2/3] ������[*] ��[o] С��[.]\n", turnID, aliveCount);
            for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                const Player &p = players[_];
                printf("[���%d(%d, %d)|����%d|�ӳ�ʣ��غ�%d|%s]\n",
                    _, p.row, p.col, p.strength, p.powerUpLeft, p.dead ? "����" : "���");
            }
            putchar(' ');
            putchar(' ');
            for (int c = 0; c < width; c++)
                printf("  %d ", c);
            putchar('\n');
            for (int r = 0; r < height; r++)
            {
                putchar(' ');
                putchar(' ');
                for (int c = 0; c < width; c++)
                {
                    putchar(' ');
                    printf((fieldStatic[r][c] & wallNorth) ? "---" : "   ");
                }
                printf("\n%d ", r);
                for (int c = 0; c < width; c++)
                {
                    putchar((fieldStatic[r][c] & wallWest) ? '|' : ' ');
                    putchar(' ');
                    int hasPlayer = -1;
                    for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
                        if (fieldContent[r][c] & playerID2Mask[_])
                            if (hasPlayer == -1)
                                hasPlayer = _;
                            else
                                hasPlayer = 4;
                    if (hasPlayer == 4)
                        putchar('*');
                    else if (hasPlayer != -1)
                        putchar('0' + hasPlayer);
                    else if (fieldStatic[r][c] & generator)
                        putchar('G');
                    else if (fieldContent[r][c] & playerMask)
                        putchar('*');
                    else if (fieldContent[r][c] & smallFruit)
                        putchar('.');
                    else if (fieldContent[r][c] & largeFruit)
                        putchar('o');
                    else
                        putchar(' ');
                    putchar(' ');
                }
                putchar((fieldStatic[r][width - 1] & wallEast) ? '|' : ' ');
                putchar('\n');
            }
            putchar(' ');
            putchar(' ');
            for (int c = 0; c < width; c++)
            {
                putchar(' ');
                printf((fieldStatic[height - 1][c] & wallSouth) ? "---" : "   ");
            }
            putchar('\n');
#endif
        }
 
        Json::Value SerializeCurrentTurnChange()
        {
            Json::Value result;
            TurnStateTransfer &bt = backtrack[turnID - 1];
            for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                result["actions"][_] = bt.actions[_];
                result["strengthDelta"][_] = bt.strengthDelta[_];
                result["change"][_] = bt.change[_];
            }
            return result;
        }
 
        // ��ʼ����Ϸ������
        GameField()
        {
            if (constructed)
                throw runtime_error("�벻Ҫ�ٴ��� GameField �����ˣ�����������ֻӦ����һ������");
            constructed = true;
 
            turnID = 0;
        }
 
        GameField(const GameField &b) : GameField() { }
    };
 
    bool GameField::constructed = false;
}
namespace Exia
{
    typedef std::pair<int, int>II;
    std::list<II>L;
    bool searched[20][20];

    const int dx[10] = { 0, 1, 0, -1, 1, 1, -1, -1 };
    const int dy[10] = { -1, 0, 1, 0, -1, 1, 1, -1 };
    int height, width;

    void check(II &nxt) {
        if (nxt.first < 0)            nxt.first += height;
        if (nxt.first >= height)      nxt.first -= height;
        if (nxt.second < 0)           nxt.second += width;
        if (nxt.second >= width)      nxt.second -= width;
    }
    int sumstep;
    int Dfs(Pacman::GameField &G, II now, II target, int step) {
        if (step > sumstep)     return -1000;
        if (now == target) {
            sumstep = step;
            return -1;
        }

        searched[now.first][now.second] = true;

        int sum_dir = -1000;
        for (int dir = 3; dir >= 0; dir--) {
            if (G.fieldStatic[now.first][now.second] & (1 << dir))      continue;   //ײǽ
            II nxt;
            nxt.first = now.first + dy[dir];
            nxt.second = now.second + dx[dir];

            check(nxt);
            
            if (searched[nxt.first][nxt.second])        continue;

            int res = Dfs(G, nxt, target, step + 1);

            if (res == -1000)                       continue;

            sum_dir = dir;
       }

        searched[now.first][now.second] = false;
        return sum_dir;
    }
    int get_neighborhood(Pacman::GameField &G, const II st) {
        L.clear();
        L.push_back(st);


        memset(searched, false, sizeof(searched));

        searched[st.first][st.second] = true;
    
        int total = 1;

        while (!L.empty()) {
            II now = L.front();     L.pop_front();
            for (int dir = 3; dir >= 0; dir--) {
                II nxt;
                nxt.first = now.first + dy[dir];
                nxt.second = now.second + dx[dir];

                if (G.fieldStatic[now.first][now.second] & (1 << dir))      continue;   //ײǽ

                check(nxt);

                if (searched[nxt.first][nxt.second])    continue;
                searched[nxt.first][nxt.second] = true;

                if ((G.fieldContent[nxt.first][nxt.second] & 16) || 
                    (G.fieldContent[nxt.first][nxt.second] & 32)) {
                    total++;
                    L.push_back(nxt);
                }
            }
        }
        memset(searched, false, sizeof(searched));
        return total;
    }

    int max(int x, int y){return x > y ? x : y;}

    int Find_road(Pacman::GameField &G, II now, II target) {
        if (now == target)      return 0;

        searched[now.first][now.second] = true;

        int res = 1000000;
        for (int i = 3; i >= 0; i--) {
            II nxt;

            if (G.fieldStatic[now.first][now.second] & (1 << i))      continue;   //ײǽ

            nxt.first = now.first + dy[i];
            nxt.second = now.second + dx[i];


            check(nxt);

            if (searched[nxt.first][nxt.second])                        continue;

            int sum = Find_road(G, nxt, target);
            if (sum == 1000000)                                         continue;

            if (sum == -1) {
                res = -1;
                break;
            }
            if (res != 1000000) {
                res = -1;
                break;
            }
            res = 1 + sum;
        }

        searched[now.first][now.second] = false;
        return res;
    }
    int calc_power(Pacman::GameField &G, int id, int turn) {
        if (G.players[id].powerUpLeft != 0 && turn > G.players[id].powerUpLeft)
            return G.players[id].strength - G.LARGE_FRUIT_ENHANCEMENT;
        else
            return G.players[id].strength;
    }
    struct node {
        II cor;
        int dir;
        int dis;
    };
    int calc_shortest_near(Pacman::GameField &G, int nowx, int nowy) {

        int res = 100000;
        for (int i = 0; i < G.generatorCount; i++) {
            II now(G.generators[i].row, G.generators[i].col);

            for (int dir = 0; dir <= 7; dir++) {
                II nxt;

                nxt.first = now.first + dy[dir];
                nxt.second = now.second + dx[dir];
                if (!((0 <= nxt.first && nxt.first < height) ||
                    (0 <= nxt.second && nxt.second < width)))
                    continue;
                check(nxt);

                bool error = false;
                for (int j = 0; j < G.generatorCount; j++) {
                    if (nxt.first == G.generators[j].row &&
                        nxt.second == G.generators[j].col)
                        error = true;
                }
                if (error)      continue;
                    
                bool bad_point = false;
                for (int kkk = 0; kkk < 4; kkk++) {
                    if ((G.fieldStatic[nxt.first][nxt.second] | (1 << kkk)) == 15)
                        bad_point = true;
                }
                if (bad_point)  continue;
                    
                sumstep = 100000;
                Dfs(G, II(nowx, nowy), nxt, 0);
                if (sumstep < res)
                    res = sumstep;
            }
        }
        return res;
    }  
    bool cmp(node A, node B){return A.dis < B.dis;}
    int my_strategy(Pacman::GameField &G, int myID) {
        height = G.height;
        width = G.width;
        //
        //printf("%d %d\n", G.players[myID].row, G.players[myID].col);
        //printf("%d\n", G.turnID);
        //
        bool warning[5], must_move[5];
        bool MM = false;
        memset(warning, false, sizeof(warning));
        memset(must_move, false, sizeof(must_move));
        for (int i = 3; i >= 0; i--) {
            II now(G.players[myID].row, G.players[myID].col);

            if (G.fieldStatic[now.first][now.second] & (1 << i))      continue;   //ײǽ
            
            II nxt;
            nxt.first = now.first + dy[i];
            nxt.second = now.second + dx[i];
            check(nxt);


            bool meet[5];       memset(meet, false, sizeof(meet));
            for (int k = 0; k < MAX_PLAYER_COUNT; k++) {
                if (k == myID)          continue;
                if (G.players[k].dead)  continue;

                if (nxt.first == G.players[k].row &&
                    nxt.second == G.players[k].col) {

                    meet[k] = true;

                    if (calc_power(G, k, 1) > 
                        calc_power(G, myID, 1)) {
                        MM = true;
                        must_move[i] = true;
                    }
                }
                for (int dir = 3; dir >= 0; dir--) {

                    II enemy_now(G.players[k].row, G.players[k].col);

                    if (G.fieldStatic[enemy_now.first][enemy_now.second] & (1 << dir))      continue;   //ײǽ
                    II enemy_nxt;
                    enemy_nxt.first = enemy_now.first + dy[dir];
                    enemy_nxt.second = enemy_now.second + dx[dir];
                    check(enemy_nxt);

                    if (nxt.first == enemy_nxt.first &&
                        nxt.second == enemy_nxt.second)
                        meet[k] = true;
                }
            }


            for (int k = 0; k < MAX_PLAYER_COUNT; k++) {
                if (k == myID)          continue;
                if (meet[k] == false)   continue;

                if (calc_power(G, myID, 1) < calc_power(G, k, 1))
                    warning[i] = true;
            }

        }

        int eat_enemy = -1;
        for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
            if (i == myID)              continue;
            if (G.players[i].dead)      continue;
            memset(searched, false, sizeof(searched));
            int sum = Find_road(G, II(G.players[myID].row, G.players[myID].col),  II(G.players[i].row, G.players[i].col));

            if (sum == 1000000 || sum == -1)    continue;
            if (calc_power(G, myID, sum) >
                calc_power(G, i, sum)) {
                
                sumstep = 100000;
                int maybe = Dfs(G, II(G.players[myID].row, G.players[myID].col), 
                                   II(G.players[i].row, G.players[i].col), 0);

                if (warning[maybe])         continue;

                if (eat_enemy == -1)        
                    eat_enemy = i;
                else if (calc_power(G, i, 0) >
                         calc_power(G, eat_enemy, 0))
                    eat_enemy = i;
            }
        }

        if (eat_enemy != -1) {
            sumstep = 100000;
            return Dfs(G, II(G.players[myID].row, G.players[myID].col), 
                          II(G.players[eat_enemy].row, G.players[eat_enemy].col), 0);
        }

        int shortest[20][20], id[20][20];
        for (int i = 0; i <= 15; i++)
            for (int j = 0; j <= 15; j++)
                shortest[i][j] = INF;

        memset(id, -1, sizeof(id));

        for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
            if (G.players[i].dead)      continue;

            int now_shortest[20][20];

            for (int i = 0; i <= 15; i++)
                for (int j = 0; j <= 15; j++)
                    now_shortest[i][j] = INF;

            L.clear();
            L.push_back(II(G.players[i].row, G.players[i].col));
            now_shortest[G.players[i].row][G.players[i].col] = 0;

            while (!L.empty()) {
                II now = L.front();     L.pop_front();
                for (int dir = 3; dir >= 0; dir--) {

                    if (G.fieldStatic[now.first][now.second] & (1 << dir))      continue;   //ײǽ
                    II nxt;
                    nxt.first = now.first + dy[dir];
                    nxt.second = now.second + dx[dir];

                    check(nxt);


                    if (now_shortest[nxt.first][nxt.second] > now_shortest[now.first][now.second] + 1) {
                        now_shortest[nxt.first][nxt.second] = now_shortest[now.first][now.second] + 1;
                        L.push_back(nxt);
                    }
                }
            }


           for (int j = height - 1; j >= 0; j--) {
                for (int k = width - 1; k >= 0; k--) {
                    if (now_shortest[j][k] < shortest[j][k] ||
                       (id[j][k] != -1 && now_shortest[j][k] == shortest[j][k] && calc_power(G, i, shortest[j][k]) >  calc_power(G, id[j][k], shortest[j][k]))
                       || (id[j][k] != -1 && now_shortest[j][k] == shortest[j][k] && calc_power(G, i, shortest[j][k]) ==  calc_power(G, id[j][k], shortest[j][k]) && i == myID) 
                       ) {
                        shortest[j][k] = now_shortest[j][k];
                        id[j][k] = i;
                    }
                }
            }
        }
        II canEat[1010];
        int NUM = 0;
        double value[1010];

        for (int i = height - 1; i >= 0; i--) {
            for (int j = width - 1; j >= 0; j--) {
                if (id[i][j] != myID)       continue;


                if ((G.fieldContent[i][j] & 16) ||
                    (G.fieldContent[i][j] & 32)) {

                    if (shortest[i][j] + calc_shortest_near(G, i, j) > 20 - G.turnID % 20) {
                        if (!((G.fieldContent[i][j] & 32) && shortest[i][j] + G.turnID > 91 && shortest[i][j] + G.turnID <= 101))
                            continue;
                    }
                    NUM++;


                    canEat[NUM] = II(i, j);
                    value[NUM] = get_neighborhood(G, canEat[NUM]);
                    if (G.fieldContent[i][j] & 32)
                        value[NUM] = (value[NUM] - 1) + 1 * (6 * 7 / (double)(height * width));

                    if ((G.fieldContent[i][j] & 32) && shortest[i][j] + G.turnID > 91 && shortest[i][j] + G.turnID <= 101)
                        value[NUM] += 100000;
                    value[NUM] /= (log((double)(shortest[i][j] + 1.001)) / log(1.5));
                }
            }
        }
        int target = -1;
        for (int i = 1; i <= NUM; i++) {

            sumstep = 100000;
            if (G.players[myID].row == canEat[i].first && G.players[myID].col == canEat[i].second)     continue;
            if (warning[Dfs(G, II(G.players[myID].row, G.players[myID].col), canEat[i], 0)])      continue;
            
            if (target == -1)
                target = i;
            else if (value[i] > value[target])
                target = i;
        }


        if (target == -1) {

            II ed(-1, -1);
            int Last = 100000;
            for (int i = 0; i < G.generatorCount; i++) {
                II now(G.generators[i].row, G.generators[i].col);

                for (int dir = 0; dir <= 7; dir++) {
                    II nxt;

                    nxt.first = now.first + dy[dir];
                    nxt.second = now.second + dx[dir];
                    if (!((0 <= nxt.first && nxt.first < height) ||
                        (0 <= nxt.second && nxt.second < width)))
                        continue;
                    check(nxt);

                    bool error = false;
                    for (int j = 0; j < G.generatorCount; j++) {
                        if (nxt.first == G.generators[j].row &&
                            nxt.second == G.generators[j].col)
                            error = true;
                    }
                    if (error)      continue;

                    if (G.players[myID].row == nxt.first &&
                        G.players[myID].col == nxt.second && MM)        continue;

                    bool bad_point = false;
                    for (int kkk = 0; kkk < 4; kkk++) {
                        if ((G.fieldStatic[nxt.first][nxt.second] | (1 << kkk)) == 15)
                            bad_point = true;
                    }
                    if (bad_point)  continue;
                    
                    if (ed.first == -1 && ed.second == -1) {
                        sumstep = 100000;
                        if (warning[Dfs(G, II(G.players[myID].row, G.players[myID].col), nxt, 0)])    continue;
                        Last = sumstep;
                        ed = nxt;
                    }
                    else {
                        sumstep = 100000;
                        if (warning[Dfs(G, II(G.players[myID].row, G.players[myID].col), nxt, 0)])   continue;

                        sumstep = Last;
                        Dfs(G, II(G.players[myID].row, G.players[myID].col), nxt, 0);
                        int New = sumstep;

                        if (New < Last) {
                            Last = New;
                            ed = nxt;
                        }
                    }

                }
            }

            NUM = 0;
            node uu[20 * 20];
            for (int j = height - 1; j >= 0; j--) {
                for (int k = width - 1; k >= 0; k--) {
                    if ((G.fieldContent[j][k] & 16) ||
                        (G.fieldContent[j][k] & 32)) {

                        NUM++;
                        uu[NUM].cor = II(j, k);

                        sumstep = 100000;
                        uu[NUM].dir = Dfs(G, II(G.players[myID].row, G.players[myID].col), uu[NUM].cor, 0);
                        if (warning[uu[NUM].dir] || (MM && j == G.players[myID].row && G.players[myID].col == k)) {
                            NUM--;
                            continue;
                        }
                        uu[NUM].dis = sumstep;
                    }
                }
            }
            std::sort(uu + 1, uu + 1 + NUM, cmp);

            if ((Last + 1 >= 20 - G.turnID % 20) || NUM == 0) {
                if ((ed.first == -1 && ed.second == -1) ||
                    (G.players[myID].row == ed.first && G.players[myID].col == ed.second)) {

                    int beiyong[5], num = 0;
                    for (int i = 3; i >= 0; i--) {
                        if (must_move[i] == false && (G.fieldStatic[G.players[myID].row][G.players[myID].col] & (1 << i)) == 0)
                            beiyong[++num] = i;
                    }

                    if (num == 0 || MM == false)       return -1;
                    else                return beiyong[rand() % num + 1];
                }

                sumstep = 100000;
                return Dfs(G, II(G.players[myID].row, G.players[myID].col), ed, 0);
            }
            else {
                return uu[1].dir;
            }
        }

        memset(searched, false, sizeof(searched));

        //kkk = canEat[target].first) +  string(canEat[target].second);

        if (canEat[target].first == G.players[myID].row &&
            canEat[target].second == G.players[myID].col)
            return -1;

        sumstep = 100000;
        return Dfs(G, II(G.players[myID].row, G.players[myID].col), canEat[target], 0);
    }
}
int main()
{
    //freopen("input.txt", "r", stdin);
    Pacman::GameField gameField;
    string data, globalData; // ���ǻغ�֮����Դ��ݵ���Ϣ
 
    // ����ڱ��ص��ԣ���input.txt����ȡ�ļ�������Ϊ����
    // �����ƽ̨�ϣ��򲻻�ȥ�������input.txt
    int myID = gameField.ReadInput("input.txt", data, globalData); // ���룬������Լ�ID
    srand(Pacman::seed + myID);
 
    //gameField.DebugPrint();
    int dir = Exia :: my_strategy(gameField, myID);
    //printf("%d\n", dir);
    gameField.WriteOutput((Pacman::Direction)(dir), "", data, globalData);
    return 0;
}
