#pragma once
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <vector>

// Uppercase = white (you, bottom), lowercase = black (Shiro, top). '.' = empty.
// Full legal-move rules: check, checkmate, stalemate, castling, en passant,
// promotion (auto-queen). Shiro: iterative deepening negamax, alpha-beta/PVS, transposition table,
// quiescence search, move ordering, repetition avoidance, tuned evaluation.
class MiniChess
{
public:
    struct Move
    {
        int fr, fc, tr, tc;
        char promo = 0;
        bool ep = false;
        bool castle = false;
    };

    MiniChess() { reset(); }

    void reset()
    {
        const char *back = "rnbqkbnr";
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                pos_.b[r][c] = '.';
        for (int c = 0; c < 8; c++)
        {
            pos_.b[0][c] = back[c];
            pos_.b[1][c] = 'p';
            pos_.b[6][c] = 'P';
            pos_.b[7][c] = static_cast<char>(std::toupper(static_cast<unsigned char>(back[c])));
        }
        pos_.wK = pos_.wQ = pos_.bK = pos_.bQ = true;
        pos_.epR = pos_.epC = -1;
        pos_.whiteToMove = true;
        selR_ = selC_ = -1;
        winner_ = 0;
        aiTimer_ = 0.f;
        tt_.clear();
        path_.clear();
        path_.push_back(hashPos(pos_));
        for (auto &row : history_)
            for (int &h : row)
                h = 0;
        for (auto &k : killers_)
            k[0] = k[1] = -1;
    }

    // difficulty knobs: Shiro stops at whichever limit is hit first
    void setThinkTime(float seconds) { thinkTime_ = seconds; }
    void setMaxDepth(int plies) { maxDepth_ = plies; }

    char at(int r, int c) const { return pos_.b[r][c]; }
    bool hasSelection() const { return selR_ >= 0; }
    int selRow() const { return selR_; }
    int selCol() const { return selC_; }
    int winner() const { return winner_; } // 0 none, 1 you (checkmate), 2 Shiro (checkmate), 3 draw (stalemate)
    bool playerTurn() const { return pos_.whiteToMove && winner_ == 0; }
    bool inCheck() const { return kingAttacked(pos_, pos_.whiteToMove); }

    static bool isWhite(char p) { return p >= 'A' && p <= 'Z'; }

    // legal moves for the piece on (r, c)
    std::vector<Move> movesFrom(int r, int c) const
    {
        std::vector<Move> out;
        if (pos_.b[r][c] == '.')
            return out;

        bool w = isWhite(pos_.b[r][c]);
        std::vector<Move> pseudo;
        gen(pos_, r, c, pseudo);

        for (const Move &m : pseudo)
        {
            Pos q = pos_;
            makeMove(q, m);
            if (!kingAttacked(q, w))
                out.push_back(m);
        }
        return out;
    }

    // player click on a square
    void click(int r, int c)
    {
        if (!playerTurn() || r < 0 || r > 7 || c < 0 || c > 7)
            return;

        if (selR_ >= 0)
            for (const Move &m : movesFrom(selR_, selC_))
                if (m.tr == r && m.tc == c)
                {
                    apply(m);
                    selR_ = selC_ = -1;
                    aiTimer_ = 0.25f; // short pause so the board redraws before Shiro thinks
                    return;
                }

        if (pos_.b[r][c] != '.' && isWhite(pos_.b[r][c]))
        {
            selR_ = r;
            selC_ = c;
        }
        else
            selR_ = selC_ = -1;
    }

    // call every frame: runs Shiro's reply
    void update(float dt)
    {
        if (winner_ || pos_.whiteToMove)
            return;

        aiTimer_ -= dt;
        if (aiTimer_ > 0.f)
            return;

        std::vector<Move> moves = allLegal(pos_, false);
        if (moves.empty())
            return; // apply() already ended the game

        apply(searchBest(moves));
    }

private:
    struct Pos
    {
        char b[8][8];
        bool wK, wQ, bK, bQ; // castling rights
        int epR, epC;        // en passant target square, -1 if none
        bool whiteToMove;
    };

    Pos pos_;
    int selR_ = -1, selC_ = -1;
    int winner_ = 0;
    float aiTimer_ = 0.f;

    // ---- engine state ----
    static constexpr int INF = 100000, MATE = 30000, MAXPLY = 64;
    enum : uint8_t { T_EXACT, T_LOWER, T_UPPER };
    struct TTEntry
    {
        uint64_t key = 0;
        int score = 0;
        int8_t depth = -1;
        uint8_t flag = 0, from = 255, to = 255;
    };
    static constexpr size_t TT_SIZE = 1u << 19; // power of two
    std::vector<TTEntry> tt_;
    std::vector<uint64_t> path_; // hashes of the game so far + current search line
    int history_[64][64] = {};
    int killers_[MAXPLY][2] = {};
    long nodes_ = 0;
    bool stopped_ = false, useTime_ = false;
    std::chrono::steady_clock::time_point deadline_;
    float thinkTime_ = 1.0f;
    int maxDepth_ = 8;

    static constexpr int KN[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
                                     {1, -2}, {1, 2}, {2, -1}, {2, 1}};

    static bool inside(int r, int c) { return r >= 0 && r < 8 && c >= 0 && c < 8; }

    // is square (r, c) attacked by any piece of the given colour?
    static bool attacked(const Pos &p, int r, int c, bool byWhite)
    {
        int pr = byWhite ? r + 1 : r - 1;
        char pawn = byWhite ? 'P' : 'p';
        for (int dc : {-1, 1})
            if (inside(pr, c + dc) && p.b[pr][c + dc] == pawn)
                return true;

        char kn = byWhite ? 'N' : 'n';
        for (int i = 0; i < 8; i++)
        {
            int tr = r + KN[i][0], tc = c + KN[i][1];
            if (inside(tr, tc) && p.b[tr][tc] == kn)
                return true;
        }

        char king = byWhite ? 'K' : 'k';
        char rook = byWhite ? 'R' : 'r';
        char bish = byWhite ? 'B' : 'b';
        char queen = byWhite ? 'Q' : 'q';

        for (int dr = -1; dr <= 1; dr++)
            for (int dc = -1; dc <= 1; dc++)
            {
                if (!dr && !dc)
                    continue;

                bool diag = dr && dc;
                for (int tr = r + dr, tc = c + dc, step = 1; inside(tr, tc); tr += dr, tc += dc, step++)
                {
                    char q = p.b[tr][tc];
                    if (q == '.')
                        continue;

                    if (q == queen || q == (diag ? bish : rook) || (step == 1 && q == king))
                        return true;
                    break;
                }
            }
        return false;
    }

    static bool kingAttacked(const Pos &p, bool white)
    {
        char k = white ? 'K' : 'k';
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                if (p.b[r][c] == k)
                    return attacked(p, r, c, !white);
        return false;
    }

    // pseudo-legal moves (may leave own king in check; castling already checks its squares)
    static void gen(const Pos &p, int r, int c, std::vector<Move> &out)
    {
        char pc = p.b[r][c];
        bool w = isWhite(pc);

        auto enemy = [&](char q)
        { return q != '.' && isWhite(q) != w; };
        auto add = [&](int tr, int tc)
        {
            if (inside(tr, tc) && (p.b[tr][tc] == '.' || enemy(p.b[tr][tc])))
                out.push_back({r, c, tr, tc});
        };
        auto slide = [&](int dr, int dc)
        {
            for (int tr = r + dr, tc = c + dc; inside(tr, tc); tr += dr, tc += dc)
            {
                if (p.b[tr][tc] == '.')
                {
                    out.push_back({r, c, tr, tc});
                    continue;
                }
                if (enemy(p.b[tr][tc]))
                    out.push_back({r, c, tr, tc});
                break;
            }
        };

        switch (std::tolower(static_cast<unsigned char>(pc)))
        {
        case 'p':
        {
            int d = w ? -1 : 1, start = w ? 6 : 1;
            auto addPawn = [&](int tr, int tc, bool ep)
            {
                Move m{r, c, tr, tc};
                m.ep = ep;
                if (tr == 0 || tr == 7)
                    m.promo = w ? 'Q' : 'q';
                out.push_back(m);
            };

            if (inside(r + d, c) && p.b[r + d][c] == '.')
            {
                addPawn(r + d, c, false);
                if (r == start && p.b[r + 2 * d][c] == '.')
                    addPawn(r + 2 * d, c, false);
            }
            for (int dc : {-1, 1})
            {
                int tc = c + dc;
                if (!inside(r + d, tc))
                    continue;
                if (enemy(p.b[r + d][tc]))
                    addPawn(r + d, tc, false);
                else if (p.epR == r + d && p.epC == tc)
                    addPawn(r + d, tc, true);
            }
            break;
        }
        case 'n':
            for (int i = 0; i < 8; i++)
                add(r + KN[i][0], c + KN[i][1]);
            break;
        case 'k':
            for (int dr = -1; dr <= 1; dr++)
                for (int dc = -1; dc <= 1; dc++)
                    if (dr || dc)
                        add(r + dr, c + dc);

            if (w ? (r == 7 && c == 4) : (r == 0 && c == 4))
            {
                bool kSide = w ? p.wK : p.bK;
                bool qSide = w ? p.wQ : p.bQ;
                char rk = w ? 'R' : 'r';

                if (!attacked(p, r, 4, !w))
                {
                    if (kSide && p.b[r][5] == '.' && p.b[r][6] == '.' && p.b[r][7] == rk &&
                        !attacked(p, r, 5, !w) && !attacked(p, r, 6, !w))
                    {
                        Move m{r, 4, r, 6};
                        m.castle = true;
                        out.push_back(m);
                    }
                    if (qSide && p.b[r][1] == '.' && p.b[r][2] == '.' && p.b[r][3] == '.' &&
                        p.b[r][0] == rk && !attacked(p, r, 3, !w) && !attacked(p, r, 2, !w))
                    {
                        Move m{r, 4, r, 2};
                        m.castle = true;
                        out.push_back(m);
                    }
                }
            }
            break;
        case 'r':
            slide(1, 0), slide(-1, 0), slide(0, 1), slide(0, -1);
            break;
        case 'b':
            slide(1, 1), slide(1, -1), slide(-1, 1), slide(-1, -1);
            break;
        case 'q':
            slide(1, 0), slide(-1, 0), slide(0, 1), slide(0, -1);
            slide(1, 1), slide(1, -1), slide(-1, 1), slide(-1, -1);
            break;
        }
    }

    static void makeMove(Pos &p, const Move &m)
    {
        char pc = p.b[m.fr][m.fc];

        if (m.ep)
            p.b[m.fr][m.tc] = '.'; // the captured pawn sits beside the start square

        if (m.castle)
        {
            if (m.tc == 6)
            {
                p.b[m.fr][5] = p.b[m.fr][7];
                p.b[m.fr][7] = '.';
            }
            else
            {
                p.b[m.fr][3] = p.b[m.fr][0];
                p.b[m.fr][0] = '.';
            }
        }

        p.b[m.fr][m.fc] = '.';
        p.b[m.tr][m.tc] = m.promo ? m.promo : pc;

        p.epR = p.epC = -1;
        if ((pc == 'P' || pc == 'p') && std::abs(m.tr - m.fr) == 2)
        {
            p.epR = (m.fr + m.tr) / 2;
            p.epC = m.fc;
        }

        // castling rights: king/rook moved, or a rook got captured on its corner
        if (m.fr == 7 && m.fc == 4)
            p.wK = p.wQ = false;
        if (m.fr == 0 && m.fc == 4)
            p.bK = p.bQ = false;
        if ((m.fr == 7 && m.fc == 0) || (m.tr == 7 && m.tc == 0))
            p.wQ = false;
        if ((m.fr == 7 && m.fc == 7) || (m.tr == 7 && m.tc == 7))
            p.wK = false;
        if ((m.fr == 0 && m.fc == 0) || (m.tr == 0 && m.tc == 0))
            p.bQ = false;
        if ((m.fr == 0 && m.fc == 7) || (m.tr == 0 && m.tc == 7))
            p.bK = false;

        p.whiteToMove = !p.whiteToMove;
    }

    static void pseudoAll(const Pos &p, bool white, std::vector<Move> &out)
    {
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                if (p.b[r][c] != '.' && isWhite(p.b[r][c]) == white)
                    gen(p, r, c, out);
    }

    static void keepLegal(const Pos &p, bool white, std::vector<Move> &ms)
    {
        size_t j = 0;
        for (size_t i = 0; i < ms.size(); i++)
        {
            Pos q = p;
            makeMove(q, ms[i]);
            if (!kingAttacked(q, white))
                ms[j++] = ms[i];
        }
        ms.erase(ms.begin() + j, ms.end());
    }

    static std::vector<Move> allLegal(const Pos &p, bool white)
    {
        std::vector<Move> out;
        pseudoAll(p, white, out);
        keepLegal(p, white, out);
        return out;
    }

    // ---------------- Shiro's brain ----------------
    static int value(char p)
    {
        switch (std::tolower(static_cast<unsigned char>(p)))
        {
        case 'p': return 100;
        case 'n': return 320;
        case 'b': return 330;
        case 'r': return 500;
        case 'q': return 900;
        default:  return 0;
        }
    }

    // Zobrist hashing
    struct Zobrist
    {
        uint64_t pc[12][64], side, castle[4], ep[8];
    };
    static const Zobrist &zob()
    {
        static const Zobrist z = []
        {
            Zobrist t;
            uint64_t s = 0x9E3779B97F4A7C15ull;
            auto next = [&]
            {
                uint64_t x = (s += 0x9E3779B97F4A7C15ull);
                x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
                x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
                return x ^ (x >> 31);
            };
            for (auto &a : t.pc)
                for (auto &k : a)
                    k = next();
            t.side = next();
            for (auto &k : t.castle)
                k = next();
            for (auto &k : t.ep)
                k = next();
            return t;
        }();
        return z;
    }

    static uint64_t hashPos(const Pos &p)
    {
        static const char *names = "PNBRQKpnbrqk";
        const Zobrist &z = zob();
        uint64_t h = 0;
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
            {
                char q = p.b[r][c];
                if (q == '.')
                    continue;
                int idx = 0;
                while (names[idx] != q)
                    idx++;
                h ^= z.pc[idx][r * 8 + c];
            }
        if (p.whiteToMove) h ^= z.side;
        if (p.wK) h ^= z.castle[0];
        if (p.wQ) h ^= z.castle[1];
        if (p.bK) h ^= z.castle[2];
        if (p.bQ) h ^= z.castle[3];
        if (p.epC >= 0) h ^= z.ep[p.epC];
        return h;
    }

    // Static evaluation from the side-to-move's point of view.
    // Material + piece-square tables + bishop pair + passed pawns +
    // rook files + king pawn-shield (fades in the endgame) + tempo.
    static int eval(const Pos &p)
    {
        // rows: P N B R Q K(midgame) K(endgame); index 0 = rank 8 (top of the board)
        static const int T[7][64] = {
            {0,0,0,0,0,0,0,0, 50,50,50,50,50,50,50,50, 10,10,20,30,30,20,10,10, 5,5,10,25,25,10,5,5,
             0,0,0,20,20,0,0,0, 5,-5,-10,0,0,-10,-5,5, 5,10,10,-20,-20,10,10,5, 0,0,0,0,0,0,0,0},
            {-50,-40,-30,-30,-30,-30,-40,-50, -40,-20,0,0,0,0,-20,-40, -30,0,10,15,15,10,0,-30, -30,5,15,20,20,15,5,-30,
             -30,0,15,20,20,15,0,-30, -30,5,10,15,15,10,5,-30, -40,-20,0,5,5,0,-20,-40, -50,-40,-30,-30,-30,-30,-40,-50},
            {-20,-10,-10,-10,-10,-10,-10,-20, -10,0,0,0,0,0,0,-10, -10,0,5,10,10,5,0,-10, -10,5,5,10,10,5,5,-10,
             -10,0,10,10,10,10,0,-10, -10,10,10,10,10,10,10,-10, -10,5,0,0,0,0,5,-10, -20,-10,-10,-10,-10,-10,-10,-20},
            {0,0,0,0,0,0,0,0, 5,10,10,10,10,10,10,5, -5,0,0,0,0,0,0,-5, -5,0,0,0,0,0,0,-5,
             -5,0,0,0,0,0,0,-5, -5,0,0,0,0,0,0,-5, -5,0,0,0,0,0,0,-5, 0,0,0,5,5,0,0,0},
            {-20,-10,-10,-5,-5,-10,-10,-20, -10,0,0,0,0,0,0,-10, -10,0,5,5,5,5,0,-10, -5,0,5,5,5,5,0,-5,
             0,0,5,5,5,5,0,-5, -10,5,5,5,5,5,0,-10, -10,0,5,0,0,0,0,-10, -20,-10,-10,-5,-5,-10,-10,-20},
            {-30,-40,-40,-50,-50,-40,-40,-30, -30,-40,-40,-50,-50,-40,-40,-30, -30,-40,-40,-50,-50,-40,-40,-30,
             -30,-40,-40,-50,-50,-40,-40,-30, -20,-30,-30,-40,-40,-30,-30,-20, -10,-20,-20,-20,-20,-20,-20,-10,
             20,20,0,0,0,0,20,20, 20,30,10,0,0,10,30,20},
            {-50,-40,-30,-20,-20,-30,-40,-50, -30,-20,-10,0,0,-10,-20,-30, -30,-10,20,30,30,20,-10,-30, -30,-10,30,40,40,30,-10,-30,
             -30,-10,30,40,40,30,-10,-30, -30,-10,20,30,30,20,-10,-30, -30,-30,0,0,0,0,-30,-30, -50,-30,-30,-30,-30,-30,-30,-50}};

        int wCnt[8] = {0}, bCnt[8] = {0};
        int bMinRow[8], wMaxRow[8];
        for (int i = 0; i < 8; i++)
            bMinRow[i] = 8, wMaxRow[i] = -1;
        int phase = 0, bishops[2] = {0, 0};

        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
            {
                char q = p.b[r][c];
                if (q == '.') continue;
                switch (q)
                {
                case 'P': wCnt[c]++; wMaxRow[c] = std::max(wMaxRow[c], r); break;
                case 'p': bCnt[c]++; bMinRow[c] = std::min(bMinRow[c], r); break;
                case 'N': case 'n': case 'B': case 'b': phase += 1; break;
                case 'R': case 'r': phase += 2; break;
                case 'Q': case 'q': phase += 4; break;
                }
                if (q == 'B') bishops[1]++;
                if (q == 'b') bishops[0]++;
            }
        phase = std::min(phase, 24);

        int s = 0;
        if (bishops[1] >= 2) s += 30;
        if (bishops[0] >= 2) s -= 30;

        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
            {
                char q = p.b[r][c];
                if (q == '.') continue;
                bool w = isWhite(q);
                char t = static_cast<char>(std::tolower(static_cast<unsigned char>(q)));
                int sq = (w ? r : 7 - r) * 8 + c;
                int v = 0;

                switch (t)
                {
                case 'p':
                {
                    v = 100 + T[0][sq];
                    bool passed = true;
                    for (int cc = std::max(0, c - 1); cc <= std::min(7, c + 1); cc++)
                        if (w ? bMinRow[cc] < r : wMaxRow[cc] > r)
                            passed = false;
                    if (passed)
                    {
                        int adv = w ? 6 - r : r - 1;
                        v += 10 + adv * adv * 4;
                    }
                    break;
                }
                case 'n': v = 320 + T[1][sq]; break;
                case 'b': v = 330 + T[2][sq]; break;
                case 'r':
                {
                    v = 500 + T[3][sq];
                    int own = w ? wCnt[c] : bCnt[c], opp = w ? bCnt[c] : wCnt[c];
                    if (!own) v += opp ? 8 : 15;
                    break;
                }
                case 'q': v = 900 + T[4][sq]; break;
                case 'k':
                {
                    v = (T[5][sq] * phase + T[6][sq] * (24 - phase)) / 24;
                    int shield = 0, d = w ? -1 : 1;
                    char pawn = w ? 'P' : 'p';
                    for (int cc = std::max(0, c - 1); cc <= std::min(7, c + 1); cc++)
                    {
                        if (inside(r + d, cc) && p.b[r + d][cc] == pawn) shield += 10;
                        else if (inside(r + 2 * d, cc) && p.b[r + 2 * d][cc] == pawn) shield += 5;
                    }
                    v += shield * phase / 24;
                    break;
                }
                }
                s += w ? v : -v;
            }

        return (p.whiteToMove ? s : -s) + 10;
    }

    static bool isCapture(const Pos &p, const Move &m) { return m.ep || p.b[m.tr][m.tc] != '.'; }

    struct Scored
    {
        int s;
        Move m;
    };

    // higher score = searched earlier
    int scoreMove(const Pos &p, const Move &m, int ply, int ttFrom, int ttTo) const
    {
        int from = m.fr * 8 + m.fc, to = m.tr * 8 + m.tc;
        if (from == ttFrom && to == ttTo) return 1000000;
        if (m.promo) return 150000;
        if (isCapture(p, m))
        {
            int victim = m.ep ? 100 : value(p.b[m.tr][m.tc]);
            return 100000 + victim * 16 - value(p.b[m.fr][m.fc]);
        }
        int k = from * 64 + to;
        if (ply < MAXPLY)
        {
            if (killers_[ply][0] == k) return 90000;
            if (killers_[ply][1] == k) return 89999;
        }
        return std::min(history_[from][to], 80000);
    }

    static void sortScored(std::vector<Scored> &v)
    {
        std::sort(v.begin(), v.end(), [](const Scored &a, const Scored &b) { return a.s > b.s; });
    }

    bool timeUp()
    {
        if (useTime_ && (nodes_ & 1023) == 0 && std::chrono::steady_clock::now() > deadline_)
            stopped_ = true;
        return stopped_;
    }

    static int toTT(int s, int ply) { return s > MATE - 1000 ? s + ply : (s < -MATE + 1000 ? s - ply : s); }
    static int fromTT(int s, int ply) { return s > MATE - 1000 ? s - ply : (s < -MATE + 1000 ? s + ply : s); }

    // captures (and check evasions) only, so we never stop mid-exchange
    int qsearch(const Pos &p, int alpha, int beta, int ply)
    {
        ++nodes_;
        if (timeUp()) return 0;

        bool w = p.whiteToMove;
        bool chk = kingAttacked(p, w);
        if (!chk)
        {
            int stand = eval(p);
            if (stand >= beta) return stand;
            alpha = std::max(alpha, stand);
        }
        if (ply >= MAXPLY - 1) return eval(p);

        std::vector<Move> raw;
        raw.reserve(48);
        pseudoAll(p, w, raw);
        if (!chk)
            raw.erase(std::remove_if(raw.begin(), raw.end(),
                                     [&](const Move &m) { return !isCapture(p, m) && !m.promo; }),
                      raw.end());
        keepLegal(p, w, raw);
        if (chk && raw.empty()) return -MATE + ply;

        std::vector<Scored> ms;
        ms.reserve(raw.size());
        for (const Move &m : raw)
            ms.push_back({scoreMove(p, m, MAXPLY, -1, -1), m});
        sortScored(ms);

        for (const Scored &sm : ms)
        {
            Pos q = p;
            makeMove(q, sm.m);
            int v = -qsearch(q, -beta, -alpha, ply + 1);
            if (stopped_) return 0;
            if (v >= beta) return v;
            alpha = std::max(alpha, v);
        }
        return alpha;
    }

    int negamax(const Pos &p, int depth, int alpha, int beta, int ply)
    {
        uint64_t h = hashPos(p);

        // repeated position anywhere in the recent line = draw
        size_t lim = std::min<size_t>(path_.size(), 32);
        for (size_t i = 0; i < lim; i++)
            if (path_[path_.size() - 1 - i] == h) return 0;

        path_.push_back(h);
        int v = negamaxImpl(p, depth, alpha, beta, ply, h);
        path_.pop_back();
        return v;
    }

    int negamaxImpl(const Pos &p, int depth, int alpha, int beta, int ply, uint64_t h)
    {
        ++nodes_;
        if (timeUp()) return 0;

        bool w = p.whiteToMove;
        bool chk = kingAttacked(p, w);
        if (chk && ply < MAXPLY - 2) depth++; // check extension
        if (depth <= 0 || ply >= MAXPLY - 1) return qsearch(p, alpha, beta, ply);

        TTEntry &e = tt_[h & (TT_SIZE - 1)];
        int ttFrom = -1, ttTo = -1;
        if (e.key == h)
        {
            if (e.from != 255) ttFrom = e.from, ttTo = e.to;
            if (e.depth >= depth)
            {
                int s = fromTT(e.score, ply);
                if (e.flag == T_EXACT) return s;
                if (e.flag == T_LOWER && s >= beta) return s;
                if (e.flag == T_UPPER && s <= alpha) return s;
            }
        }

        std::vector<Move> raw;
        raw.reserve(48);
        pseudoAll(p, w, raw);
        keepLegal(p, w, raw);
        if (raw.empty()) return chk ? -MATE + ply : 0;

        std::vector<Scored> ms;
        ms.reserve(raw.size());
        for (const Move &m : raw)
            ms.push_back({scoreMove(p, m, ply, ttFrom, ttTo), m});
        sortScored(ms);

        int origAlpha = alpha, best = -INF;
        int bf = 255, bt = 255;

        for (size_t i = 0; i < ms.size(); i++)
        {
            const Move &m = ms[i].m;
            bool quiet = !isCapture(p, m) && !m.promo;
            Pos q = p;
            makeMove(q, m);

            int v;
            if (i == 0)
                v = -negamax(q, depth - 1, -beta, -alpha, ply + 1);
            else
            {
                int red = (i >= 4 && depth >= 3 && quiet && !chk) ? (i >= 10 ? 2 : 1) : 0;
                v = -negamax(q, depth - 1 - red, -alpha - 1, -alpha, ply + 1);
                if (v > alpha && (red > 0 || v < beta) && !stopped_)
                    v = -negamax(q, depth - 1, -beta, -alpha, ply + 1);
            }
            if (stopped_) return 0;

            if (v > best)
            {
                best = v;
                bf = m.fr * 8 + m.fc;
                bt = m.tr * 8 + m.tc;
            }
            alpha = std::max(alpha, v);
            if (alpha >= beta)
            {
                if (quiet)
                {
                    int k = m.fr * 8 * 64 + m.fc * 64 + m.tr * 8 + m.tc;
                    if (killers_[ply][0] != k)
                    {
                        killers_[ply][1] = killers_[ply][0];
                        killers_[ply][0] = k;
                    }
                    int &hh = history_[m.fr * 8 + m.fc][m.tr * 8 + m.tc];
                    hh = std::min(hh + depth * depth, 80000);
                }
                break;
            }
        }

        e.key = h;
        e.score = toTT(best, ply);
        e.depth = static_cast<int8_t>(depth);
        e.flag = best <= origAlpha ? T_UPPER : (best >= beta ? T_LOWER : T_EXACT);
        e.from = static_cast<uint8_t>(bf);
        e.to = static_cast<uint8_t>(bt);
        return best;
    }

    // iterative deepening at the root: depth 1, 2, 3 ... until time or depth limit
    Move searchBest(std::vector<Move> root)
    {
        if (root.size() == 1) return root[0];
        if (tt_.empty()) tt_.assign(TT_SIZE, TTEntry{});

        nodes_ = 0;
        stopped_ = false;
        deadline_ = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(static_cast<int>(thinkTime_ * 1000));
        for (auto &k : killers_) k[0] = k[1] = -1;
        for (auto &row : history_)
            for (int &x : row) x /= 2;

        // small shuffle so equal-scored moves don't always play out identically
        for (size_t i = root.size(); i > 1; i--)
            std::swap(root[i - 1], root[std::rand() % i]);

        Move best = root[0];
        for (int depth = 1; depth <= maxDepth_; depth++)
        {
            useTime_ = depth > 1; // always finish depth 1
            int alpha = -INF, bestV = -INF;
            Move iterBest = root[0];

            for (const Move &m : root)
            {
                Pos q = pos_;
                makeMove(q, m);
                int v = -negamax(q, depth - 1, -INF, -alpha, 1);
                if (stopped_) break;
                if (v > bestV)
                {
                    bestV = v;
                    iterBest = m;
                }
                alpha = std::max(alpha, v);
            }
            if (stopped_) break;

            best = iterBest;
            for (size_t i = 0; i < root.size(); i++) // best move first next iteration
                if (root[i].fr == best.fr && root[i].fc == best.fc && root[i].tr == best.tr &&
                    root[i].tc == best.tc && root[i].promo == best.promo)
                {
                    std::rotate(root.begin(), root.begin() + i, root.begin() + i + 1);
                    break;
                }
            if (bestV > MATE - 100) break; // forced mate found
        }
        return best;
    }

    // plays a move for real, then checks for checkmate / stalemate
    void apply(const Move &m)
    {
        makeMove(pos_, m);
        path_.push_back(hashPos(pos_));

        bool sideToMove = pos_.whiteToMove;
        if (allLegal(pos_, sideToMove).empty())
        {
            if (kingAttacked(pos_, sideToMove))
                winner_ = sideToMove ? 2 : 1; // the side to move got checkmated
            else
                winner_ = 3; // stalemate
        }
    }
};
