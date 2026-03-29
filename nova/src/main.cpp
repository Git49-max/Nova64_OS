#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <string>
#ifdef _WIN32
    #include <ncurses/curses.h>
#else
    #include <ncurses.h>
#endif
#include <vector>
#include <set>
#include <algorithm>
#include <unistd.h>   // getcwd
#include "../include/ast.h"
#include "../include/codegen.h"
#include "../include/error.h"
#include "../include/analysis.h"

ProgramNode parseProgram(const std::string& source, const std::string& filename);

// ── Cores ANSI (para output fora do editor) ───────────────────────────────────
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_RED     "\033[1;31m"
#define ANSI_YELLOW  "\033[1;33m"
#define ANSI_CYAN    "\033[1;36m"
#define ANSI_BLUE    "\033[1;34m"
#define ANSI_GRAY    "\033[0;90m"
#define ANSI_WHITE   "\033[1;37m"

static void printError(const std::string& msg) {
    std::cerr << ANSI_RED << "  ✗ error: " << ANSI_RESET << ANSI_BOLD << msg << ANSI_RESET << "\n";
}

// ── --help ────────────────────────────────────────────────────────────────────
static void printHelp() {
    std::cout << ANSI_BOLD << "Nova Compiler - Beta 2.0.1 \n" << ANSI_RESET;
    std::cout << "\n";
    std::cout << ANSI_BOLD << "Usage:\n" << ANSI_RESET;
    std::cout << "  n++ <file.npp> [file2.npp ...] [options]\n";
    std::cout << "  n++ -w [file.npp]\n";
    std::cout << "  n++ --version\n";
    std::cout << "  n++ --help\n";
    std::cout << "\n";
    std::cout << ANSI_BOLD << "Options:\n" << ANSI_RESET;
    std::cout << ANSI_CYAN << "  -o <out>      " << ANSI_RESET << "Set output file name (.exe/.dll → Windows PE32+)\n";
    std::cout << ANSI_CYAN << "  -O2           " << ANSI_RESET << "Enable level 2 optimizations\n";
    std::cout << ANSI_CYAN << "  -O3           " << ANSI_RESET << "Enable level 3 optimizations (aggressive)\n";
    std::cout << ANSI_CYAN << "  -w [file]     " << ANSI_RESET << "Open interactive editor in terminal\n";
    std::cout << ANSI_CYAN << "  --version     " << ANSI_RESET << "Show compiler version\n";
    std::cout << ANSI_CYAN << "  --help        " << ANSI_RESET << "Show this help message\n";
    std::cout << "\n";
    std::cout << ANSI_BOLD << "Examples:\n" << ANSI_RESET;
    std::cout << ANSI_GRAY << "  n++ hello.npp\n";
    std::cout << "  n++ hello.npp -o hello -O2\n";
    std::cout << "  n++ hello.npp -o hello.exe        # cross-compile to Windows\n";
    std::cout << "  n++ hello.npp -o hello.dll        # Windows DLL\n";
    std::cout << "  n++ -w                            # new file\n";
    std::cout << "  n++ -w existing.npp               # edit existing file\n" << ANSI_RESET;
    std::cout << "\n";
    std::cout << ANSI_BOLD << "Editor shortcuts (-w mode):\n" << ANSI_RESET;
    std::cout << ANSI_GRAY << "  Ctrl+S   Save and compile\n";
    std::cout << "  Ctrl+Q   Quit without compiling\n";
    std::cout << "  Ctrl+X   Save, compile and exit\n";
    std::cout << "  Arrow keys / Home / End / PgUp / PgDn to navigate\n" << ANSI_RESET;
    std::cout << "\n";
}

// ── Detecta se o output alvo é Windows ───────────────────────────────────────
static bool isWindowsTarget(const std::string& outFile) {
    auto dot = outFile.rfind('.');
    if (dot == std::string::npos) return false;
    std::string ext = outFile.substr(dot);
    return (ext == ".exe" || ext == ".dll");
}

// ── Link command builder ──────────────────────────────────────────────────────
// objFiles: lista de todos os .o gerados
static std::string buildLinkCmd(const std::vector<std::string>& objFiles,
                                const std::string& outFile) {
    std::string ext;
    auto dot = outFile.rfind('.');
    if (dot != std::string::npos) ext = outFile.substr(dot);

    std::string objs;
    for (auto& o : objFiles) objs += " " + o;

    // ── Alvo Windows: .exe ou .dll → usa MinGW (gera PE32+)
    if (ext == ".exe" || ext == ".dll") {
        std::string cmd = "x86_64-w64-mingw32-gcc -m64" + objs;
        if (ext == ".dll") cmd += " -shared";
        cmd += " -o " + outFile;
        cmd += " -lmingw32 -lmingwex -lmsvcrt";
        return cmd;
    }

    // ── Alvo host (Linux/macOS)

    #ifdef _WIN32
        std::string cmd = "gcc -m64" + objs;
    #else
        std::string cmd = "clang -m64" + objs;
    #endif
    if (ext == ".so" || ext == ".dylib") {
        cmd += " -shared";
    } else if (ext == ".a") {
        return "ar rcs " + outFile + objs;
    }
    cmd += " -o " + outFile;
    return cmd;
}

// ── Detecta stdlib includes nos arquivos fonte ────────────────────────────────
// Varre cada .npp buscando linhas "#include <nome.nh>" e retorna os .npp
// correspondentes em NOVA_STDLIB_PATH que ainda não estão na lista de arquivos.
static std::vector<std::string> detectStdlibModules(
        const std::vector<std::string>& sourceFiles) {

    const char* envPath = std::getenv("NOVA_STDLIB_PATH");
    std::string stdlibDir = envPath ? std::string(envPath) : "/usr/local/lib/nova";

    std::vector<std::string> result;
    std::set<std::string> added; // evita duplicatas

    // Pré-popula com os arquivos já na lista (não queremos readicionar)
    for (auto& s : sourceFiles) added.insert(s);

    for (auto& src : sourceFiles) {
        std::ifstream f(src);
        if (!f) continue;
        std::string line;
        while (std::getline(f, line)) {
            // Busca padrão: #include <nome.nh>
            size_t hash = line.find('#');
            if (hash == std::string::npos) continue;
            size_t inc = line.find("include", hash + 1);
            if (inc == std::string::npos) continue;
            size_t lt = line.find('<', inc + 7);
            size_t gt = line.find('>', lt == std::string::npos ? 0 : lt + 1);
            if (lt == std::string::npos || gt == std::string::npos) continue;

            std::string headerName = line.substr(lt + 1, gt - lt - 1);
            // Troca extensão .nh → .npp
            std::string moduleName = headerName;
            if (moduleName.size() > 3 &&
                moduleName.substr(moduleName.size() - 3) == ".nh")
                moduleName = moduleName.substr(0, moduleName.size() - 3) + ".npp";

            std::string modulePath = stdlibDir + "/" + moduleName;

            // Só adiciona se o arquivo existir e ainda não estiver na lista
            if (added.count(modulePath) == 0) {
                std::ifstream check(modulePath);
                if (check.good()) {
                    result.push_back(modulePath);
                    added.insert(modulePath);
                }
            }
        }
    }
    return result;
}

// ── Compila um único arquivo .npp para .o ─────────────────────────────────────
static std::string compileSingleFile(const std::string& srcFile, int optLevel,
                                     bool winTarget, const std::string& objOut) {
    std::ifstream file(srcFile);
    if (!file) { printError("file '" + srcFile + "' not found"); return ""; }
    std::stringstream buf;
    buf << file.rdbuf();

    ProgramNode program = parseProgram(buf.str(), srcFile);
    runAnalysis(program, srcFile);
    codegenProgram(program, objOut, srcFile, optLevel, winTarget);
    return objOut;
}

// ── Compilação silenciosa (sem output a não ser erros/warnings) ───────────────
static int compileFile(const std::vector<std::string>& sourceFiles,
                       const std::string& outputFileArg, int optLevel) {
    if (sourceFiles.empty()) { printError("no source files specified"); return 1; }

    // Determina o nome de saída
    std::string outputFile = outputFileArg;
    if (outputFile.empty()) {
        // Usa o nome do primeiro arquivo sem extensão
        outputFile = sourceFiles[0];
        auto dot = outputFile.rfind('.');
        if (dot != std::string::npos) outputFile = outputFile.substr(0, dot);
    }

    bool winTarget = isWindowsTarget(outputFile);

    // Detecta e injeta automaticamente os .npp da stdlib referenciados via #include <...>
    std::vector<std::string> allFiles = sourceFiles;
    std::vector<std::string> stdlibModules = detectStdlibModules(sourceFiles);
    for (auto& m : stdlibModules)
        allFiles.push_back(m);

    // Compila cada arquivo fonte para seu próprio .o
    std::vector<std::string> objFiles;
    bool projectHasMain = false;
    for (auto& src : allFiles) {
        // .o fica em /tmp para não poluir o diretório do usuário
        std::string base = src;
        // Remove path e extensão para o nome do .o
        auto slash = base.rfind('/');
        if (slash != std::string::npos) base = base.substr(slash + 1);
        auto dot = base.rfind('.');
        if (dot != std::string::npos) base = base.substr(0, dot);
        #ifdef _WIN32
            std::string objPath = std::string(getenv("TEMP") ? getenv("TEMP") : "C:/tmp") + "/nova_" + base + "_" + std::to_string(objFiles.size()) + ".o";
        #else
            std::string objPath = "/tmp/nova_" + base + "_" + std::to_string(objFiles.size()) + ".o";
        #endif

        std::string obj = compileSingleFile(src, optLevel, winTarget, objPath);
        if (obj.empty()) return 1;
        objFiles.push_back(obj);

        // Checa se este arquivo continha main (lendo o fonte novamente é caro;
        // compileSingleFile já rodou — o codegen sabe via isLibraryFile)
        // Mais simples: fazemos uma passagem rápida pelo fonte buscando "void main" ou "int main"
        {
            std::ifstream f(src);
            std::string content((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
            if (content.find("main") != std::string::npos)
                projectHasMain = true;
        }
    }

    // Emite warnings de funções definidas mas nunca chamadas em todo o projeto
    flushFunctionWarnings(projectHasMain);

    // Linka todos os .o juntos — captura stderr do linker para exibir erros amigáveis
    std::string linkCmd = buildLinkCmd(objFiles, outputFile);

    // Redireciona stderr do linker para arquivo temporário
    // Linha 242-243, troca por:
    #ifdef _WIN32
        std::string linkerErrFile = std::string(getenv("TEMP") ? getenv("TEMP") : "C:/tmp") + "/nova_linker_" + std::to_string(getpid()) + ".txt";
        std::string fullCmd = "C:/msys64/mingw64/bin/gcc.exe -m64";
        for (auto& o : objFiles) fullCmd += " " + o;
        // rebuild linkCmd com caminho absoluto
        std::string linkCmd2 = fullCmd + " -o " + outputFile;
        int ret = system((linkCmd2 + " 2>" + linkerErrFile).c_str());
    #else
        std::string linkerErrFile = "/tmp/nova_linker_" + std::to_string(getpid()) + ".txt";
        int ret = system((linkCmd + " 2>" + linkerErrFile).c_str());
    #endif

    // Remove os .o temporários
    for (auto& o : objFiles) std::remove(o.c_str());

    if (ret != 0) {
        std::string linkerOutput;
        {
            std::ifstream ef(linkerErrFile);
            if (ef) { std::stringstream eb; eb << ef.rdbuf(); linkerOutput = eb.str(); }
        }
        std::remove(linkerErrFile.c_str());
        reportLinkerError(linkerOutput, outputFile, allFiles);
        return 1;
    }
    std::remove(linkerErrFile.c_str());
    return 0;
}

// ═════════════════════════════════════════════════════════════════════════════
// EDITOR INTERATIVO (-w)
// ═════════════════════════════════════════════════════════════════════════════

static const std::vector<std::string> KEYWORDS = {
    "if","else","while","for","return","print","struct",
    "i32","f32","string","void", "char", "i64", "f64", "i64"
};


// ── Paleta de cores do editor ─────────────────────────────────────────────────
// Fundo escuro azul-acinzentado, sem verde em lugar nenhum
enum NcColors {
    COL_NORMAL    = 1,   // texto normal        → branco frio
    COL_KEYWORD   = 2,   // keywords            → azul claro bold
    COL_TYPE      = 3,   // int float string    → ciano
    COL_NUMBER    = 4,   // literais numéricos  → amarelo
    COL_STRING    = 5,   // strings             → laranja/amarelo escuro
    COL_COMMENT   = 6,   // comentários         → cinza
    COL_LINENO    = 7,   // números de linha    → cinza azulado
    COL_STATUSBAR = 8,   // barra de status ok  → azul escuro
    COL_STATUSER  = 9,   // barra de status err → vermelho escuro
    COL_DIALOG    = 10,  // caixa de diálogo    → fundo azul médio
    COL_DIGINPUT  = 11,  // input do diálogo    → fundo ligeiramente mais claro
};

static void initEditorColors() {
    start_color();
    use_default_colors();

    // Texto do editor
    init_pair(COL_NORMAL,    COLOR_WHITE,   -1);
    init_pair(COL_KEYWORD,   COLOR_BLUE,    -1);   // azul bold = azul claro no terminal
    init_pair(COL_TYPE,      COLOR_CYAN,    -1);
    init_pair(COL_NUMBER,    COLOR_YELLOW,  -1);
    init_pair(COL_STRING,    COLOR_YELLOW,  -1);   // amarelo escuro (sem verde)
    init_pair(COL_COMMENT,   COLOR_BLACK,   -1);   // bright black = cinza

    // UI
    init_pair(COL_LINENO,    COLOR_BLUE,    -1);
    init_pair(COL_STATUSBAR, COLOR_WHITE,   COLOR_BLUE);
    init_pair(COL_STATUSER,  COLOR_WHITE,   COLOR_RED);
    init_pair(COL_DIALOG,    COLOR_WHITE,   COLOR_BLUE);
    init_pair(COL_DIGINPUT,  COLOR_WHITE,   COLOR_BLACK);
}

// Palavras-chave de tipo separadas para cor diferente
static const std::vector<std::string> TYPE_KW = {"i32","i64","char", "f32", "f64", "string","void"};
static bool isTypeKw(const std::string& w) {
    return std::find(TYPE_KW.begin(), TYPE_KW.end(), w) != TYPE_KW.end();
}
static const std::vector<std::string> CTRL_KW = {"if","else","while","for","return","print","struct", "impl"};
static bool isCtrlKw(const std::string& w) {
    return std::find(CTRL_KW.begin(), CTRL_KW.end(), w) != CTRL_KW.end();
}

// ── Renderiza uma linha com syntax highlight ──────────────────────────────────
static void renderLine(WINDOW* win, int y, int xOff,
                       const std::string& line, int scrollX) {
    wmove(win, y, xOff);
    wclrtoeol(win);

    std::string vis = (scrollX < (int)line.size()) ? line.substr(scrollX) : "";
    int maxw = getmaxx(win) - xOff;
    if ((int)vis.size() > maxw) vis = vis.substr(0, maxw);

    size_t i = 0;
    while (i < vis.size()) {
        // Comentário
        if (i + 1 < vis.size() && vis[i] == '/' && vis[i+1] == '/') {
            wattron(win, COLOR_PAIR(COL_COMMENT) | A_BOLD);
            waddstr(win, vis.substr(i).c_str());
            wattroff(win, COLOR_PAIR(COL_COMMENT) | A_BOLD);
            break;
        }
        // String
        if (vis[i] == '"') {
            wattron(win, COLOR_PAIR(COL_STRING) | A_BOLD);
            waddch(win, '"'); i++;
            while (i < vis.size() && vis[i] != '"') waddch(win, vis[i++]);
            if (i < vis.size()) { waddch(win, '"'); i++; }
            wattroff(win, COLOR_PAIR(COL_STRING) | A_BOLD);
            continue;
        }
        // Número
        if (std::isdigit((unsigned char)vis[i])) {
            wattron(win, COLOR_PAIR(COL_NUMBER) | A_BOLD);
            while (i < vis.size() &&
                   (std::isdigit((unsigned char)vis[i]) || vis[i] == '.'))
                waddch(win, vis[i++]);
            wattroff(win, COLOR_PAIR(COL_NUMBER) | A_BOLD);
            continue;
        }
        // Palavra
        if (std::isalpha((unsigned char)vis[i]) || vis[i] == '_') {
            std::string word;
            while (i < vis.size() &&
                   (std::isalnum((unsigned char)vis[i]) || vis[i] == '_'))
                word += vis[i++];
            if (isTypeKw(word)) {
                wattron(win, COLOR_PAIR(COL_TYPE) | A_BOLD);
                waddstr(win, word.c_str());
                wattroff(win, COLOR_PAIR(COL_TYPE) | A_BOLD);
            } else if (isCtrlKw(word)) {
                wattron(win, COLOR_PAIR(COL_KEYWORD) | A_BOLD);
                waddstr(win, word.c_str());
                wattroff(win, COLOR_PAIR(COL_KEYWORD) | A_BOLD);
            } else {
                wattron(win, COLOR_PAIR(COL_NORMAL));
                waddstr(win, word.c_str());
                wattroff(win, COLOR_PAIR(COL_NORMAL));
            }
            continue;
        }
        if (std::string("+-*/=%&|<>!:").find(vis[i]) != std::string::npos) {
            wattron(win, COLOR_PAIR(COL_TYPE) | A_BOLD); // Usa a cor de tipos para símbolos
            waddch(win, (unsigned char)vis[i++]);
            wattroff(win, COLOR_PAIR(COL_TYPE) | A_BOLD);
            continue;
        }
        // Normal
        wattron(win, COLOR_PAIR(COL_NORMAL));
        waddch(win, (unsigned char)vis[i++]);
        wattroff(win, COLOR_PAIR(COL_NORMAL));
    }
}

// ── Dialog de input de texto (nome do arquivo / caminho) ─────────────────────
// Retorna o texto digitado, ou "" se o usuário cancelou com Esc.
static std::string inputDialog(const std::string& prompt,
                                const std::string& preset) {
    int maxy, maxx;
    getmaxyx(stdscr, maxy, maxx);

    const int dw = std::min(maxx - 4, 72);
    const int dh = 5;
    const int dy = maxy / 2 - dh / 2;
    const int dx = maxx / 2 - dw / 2;

    WINDOW* dlgWin = newwin(dh, dw, dy, dx);
    keypad(dlgWin, TRUE);

    std::string input = preset;
    int cursor = (int)input.size();

    auto drawDialog = [&]() {
        werase(dlgWin);
        wattron(dlgWin, COLOR_PAIR(COL_DIALOG) | A_BOLD);
        box(dlgWin, 0, 0);                        // borda dupla
        // Título
        std::string title = "  " + prompt + "  ";
        mvwaddstr(dlgWin, 0, (dw - (int)title.size()) / 2, title.c_str());
        wattroff(dlgWin, COLOR_PAIR(COL_DIALOG) | A_BOLD);

        // Campo de input
        int fieldW = dw - 4;
        int fieldX = 2;
        int fieldY = 2;
        wattron(dlgWin, COLOR_PAIR(COL_DIGINPUT));
        std::string field(fieldW, ' ');
        mvwaddstr(dlgWin, fieldY, fieldX, field.c_str());

        // Calcula scroll do input para mostrar o cursor
        int scrollI = 0;
        if (cursor >= fieldW) scrollI = cursor - fieldW + 1;
        std::string visible = input.substr(scrollI);
        if ((int)visible.size() > fieldW) visible = visible.substr(0, fieldW);
        mvwaddstr(dlgWin, fieldY, fieldX, visible.c_str());
        wattroff(dlgWin, COLOR_PAIR(COL_DIGINPUT));

        // Hint
        wattron(dlgWin, COLOR_PAIR(COL_COMMENT));
        mvwaddstr(dlgWin, dh - 1, 2, " Enter: confirm   Esc: cancel ");
        wattroff(dlgWin, COLOR_PAIR(COL_COMMENT));

        // Cursor
        int curScreenX = fieldX + (cursor - scrollI);
        wmove(dlgWin, fieldY, curScreenX);
        wrefresh(dlgWin);
    };

    drawDialog();

    while (true) {
        int ch = wgetch(dlgWin);
        if (ch == 27) {          // Esc — cancela
            delwin(dlgWin);
            return "";
        }
        if (ch == '\n' || ch == KEY_ENTER) {
            delwin(dlgWin);
            return input;
        }
        if (ch == KEY_LEFT  && cursor > 0) { cursor--; }
        else if (ch == KEY_RIGHT && cursor < (int)input.size()) { cursor++; }
        else if (ch == KEY_HOME) { cursor = 0; }
        else if (ch == KEY_END)  { cursor = (int)input.size(); }
        else if ((ch == KEY_BACKSPACE || ch == 127) && cursor > 0) {
            input.erase(cursor - 1, 1);
            cursor--;
        } else if (ch == KEY_DC && cursor < (int)input.size()) {
            input.erase(cursor, 1);
        } else if (ch >= 32 && ch < 127) {
            input.insert(cursor, 1, (char)ch);
            cursor++;
        }
        drawDialog();
    }
}

// ── Retorna o diretório atual ─────────────────────────────────────────────────
static std::string currentDir() {
    char buf[4096];
    if (getcwd(buf, sizeof(buf))) return std::string(buf) + "/";
    return "";
}

// ── Editor principal ──────────────────────────────────────────────────────────
static void runEditor(const std::string& editFile,
                      const std::string& outputFileArg, int optLevel) {

    // ── Pede nome do arquivo se não foi passado ───────────────────────
    std::string filename = editFile;

    // Inicia ncurses antes do dialog para poder mostrar a UI de input
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(1);
    initEditorColors();

    if (filename.empty()) {
        std::string preset = currentDir() + "untitled.npp";
        std::string result = inputDialog(" Save as ", preset);
        if (result.empty()) {
            endwin();
            return;
        }
        filename = result;
    }

    // ── Carrega conteúdo ──────────────────────────────────────────────
    std::vector<std::string> lines;
    {
        std::ifstream f(filename);
        if (f) {
            std::string ln;
            while (std::getline(f, ln)) lines.push_back(ln);
        }
    }
    if (lines.empty()) lines.push_back("");

    int maxy, maxx;
    getmaxyx(stdscr, maxy, maxx);

    const int LINENO_WIDTH = 6;  // " 9999 "
    WINDOW* statusWin = newwin(1,        maxx, 0,        0);
    WINDOW* editorWin = newwin(maxy - 2, maxx, 1,        0);
    WINDOW* footerWin = newwin(1,        maxx, maxy - 1, 0);
    keypad(editorWin, TRUE);

    int curRow = 0, curCol = 0;
    int scrollRow = 0, scrollX = 0;
    bool dirty = false;
    std::string statusMsg;
    bool statusIsError = false;

    // ── Redraw ────────────────────────────────────────────────────────
    auto redraw = [&]() {
        getmaxyx(stdscr, maxy, maxx);
        wresize(statusWin, 1,        maxx);
        wresize(editorWin, maxy - 2, maxx);
        wresize(footerWin, 1,        maxx);
        mvwin(footerWin, maxy - 1, 0);

        int editorH = maxy - 2;
        int editorW = maxx - LINENO_WIDTH;

        // Scroll
        if (curRow < scrollRow) scrollRow = curRow;
        if (curRow >= scrollRow + editorH) scrollRow = curRow - editorH + 1;
        if (curCol < scrollX) scrollX = curCol;
        if (curCol >= scrollX + editorW) scrollX = curCol - editorW + 1;

        // ── Status bar ────────────────────────────────────────────────
        werase(statusWin);
        int scol = statusIsError ? COL_STATUSER : COL_STATUSBAR;
        wattron(statusWin, COLOR_PAIR(scol) | A_BOLD);
        // Parte esquerda: nome + flag de não salvo
        std::string left = std::string(dirty ? " ● " : "   ") + filename;
        // Parte direita: mensagem de status
        std::string right = statusMsg + "   ";
        // Preenche com espaços no meio
        int totalLen = (int)left.size() + (int)right.size();
        int spaces = maxx - totalLen;
        if (spaces < 1) spaces = 1;
        std::string bar = left + std::string(spaces, ' ') + right;
        bar.resize(maxx, ' ');
        waddstr(statusWin, bar.c_str());
        wattroff(statusWin, COLOR_PAIR(scol) | A_BOLD);
        wrefresh(statusWin);

        // ── Linhas do editor ──────────────────────────────────────────
        werase(editorWin);
        for (int r = 0; r < editorH; r++) {
            int lineIdx = scrollRow + r;
            if (lineIdx < (int)lines.size()) {
                // Número da linha
                wattron(editorWin, COLOR_PAIR(COL_LINENO) | A_BOLD);
                char lnbuf[16];
                snprintf(lnbuf, sizeof(lnbuf), "%4d  ", lineIdx + 1);
                mvwaddstr(editorWin, r, 0, lnbuf);
                wattroff(editorWin, COLOR_PAIR(COL_LINENO) | A_BOLD);
                // Conteúdo com highlight
                renderLine(editorWin, r, LINENO_WIDTH, lines[lineIdx], scrollX);
            } else {
                wattron(editorWin, COLOR_PAIR(COL_LINENO) | A_BOLD);
                mvwaddstr(editorWin, r, 0, "    ~ ");
                wattroff(editorWin, COLOR_PAIR(COL_LINENO) | A_BOLD);
            }
        }
        wmove(editorWin, curRow - scrollRow, LINENO_WIDTH + (curCol - scrollX));
        wrefresh(editorWin);
        int cursorY = curRow - scrollRow;
        int cursorX = LINENO_WIDTH + (curCol - scrollX);

        // A_BLINK faz o caractere piscar (se o terminal suportar)
        wattron(editorWin, A_BLINK | COLOR_PAIR(COL_KEYWORD) | A_BOLD);
        mvwaddch(editorWin, cursorY, cursorX, '|'); 
        wattroff(editorWin, A_BLINK | COLOR_PAIR(COL_KEYWORD) | A_BOLD);

        // Move o cursor real para a mesma posição (ajuda na navegação do terminal)
        wmove(editorWin, cursorY, cursorX);
        wrefresh(editorWin);
        // ---------------------------------

        // ── Footer ────────────────────────────────────────────────────
        werase(footerWin);
        wattron(footerWin, A_REVERSE | A_BOLD);
        char pos[64];
        snprintf(pos, sizeof(pos), " Ln %d, Col %d ", curRow + 1, curCol + 1);
        std::string shortcuts = "  ^S Save & Compile   ^X Save, Compile & Exit"
                                "   ^R Rename   ^Q Quit  ";
        std::string fright = std::string(pos);
        int fspaces = maxx - (int)shortcuts.size() - (int)fright.size();
        if (fspaces < 0) fspaces = 0;
        std::string footer = shortcuts + std::string(fspaces, ' ') + fright;
        footer.resize(maxx, ' ');
        waddstr(footerWin, footer.c_str());
        wattroff(footerWin, A_REVERSE | A_BOLD);
        wrefresh(footerWin);
    };

    // ── Save ──────────────────────────────────────────────────────────
    auto saveFile = [&]() -> bool {
        std::ofstream f(filename);
        if (!f) {
            statusMsg = "Error: could not save " + filename;
            statusIsError = true;
            return false;
        }
        for (size_t i = 0; i < lines.size(); i++) {
            f << lines[i];
            if (i + 1 < lines.size()) f << "\n";
        }
        dirty = false;
        return true;
    };

    // ── Save & Compile ────────────────────────────────────────────────
    auto saveAndCompile = [&]() -> bool {
        if (!saveFile()) return false;
        endwin();
        int ret = compileFile({filename}, outputFileArg, optLevel);
        // Reinicia ncurses
        initscr(); raw(); keypad(stdscr, TRUE); noecho(); curs_set(1);
        initEditorColors();
        if (ret == 0) {
            statusMsg = "Compiled OK";
            statusIsError = false;
        } else {
            statusMsg = "Compile error — check output above";
            statusIsError = true;
        }
        return ret == 0;
    };

    redraw();

    while (true) {
        int ch = wgetch(editorWin);

        if (curRow >= (int)lines.size()) curRow = (int)lines.size() - 1;
        if (curRow < 0) curRow = 0;
        int lineLen = (int)lines[curRow].size();
        if (curCol > lineLen) curCol = lineLen;

        if (ch == ('s' & 0x1f)) {           // Ctrl+S
            saveAndCompile();

        } else if (ch == ('x' & 0x1f)) {    // Ctrl+X — salva, compila e sai
            saveAndCompile();
            endwin();
            return;

        } else if (ch == ('q' & 0x1f)) {    // Ctrl+Q — sai
            endwin();
            return;

        } else if (ch == ('r' & 0x1f)) {    // Ctrl+R — renomeia / move arquivo
            std::string newName = inputDialog(" Save as ", filename);
            if (!newName.empty()) {
                filename = newName;
                dirty = true;
                statusMsg = "Path changed — ^S to save";
                statusIsError = false;
            }

        } else if (ch == KEY_UP) {
            if (curRow > 0) { curRow--; curCol = std::min(curCol, (int)lines[curRow].size()); }

        } else if (ch == KEY_DOWN) {
            if (curRow < (int)lines.size() - 1) { curRow++; curCol = std::min(curCol, (int)lines[curRow].size()); }

        } else if (ch == KEY_LEFT) {
            if (curCol > 0) curCol--;
            else if (curRow > 0) { curRow--; curCol = (int)lines[curRow].size(); }

        } else if (ch == KEY_RIGHT) {
            if (curCol < lineLen) curCol++;
            else if (curRow < (int)lines.size() - 1) { curRow++; curCol = 0; }

        } else if (ch == KEY_HOME)  { curCol = 0; }
        else if (ch == KEY_END)   { curCol = lineLen; }

        else if (ch == KEY_PPAGE) {
            int editorH = maxy - 2;
            curRow = std::max(0, curRow - editorH);
            curCol = std::min(curCol, (int)lines[curRow].size());

        } else if (ch == KEY_NPAGE) {
            int editorH = maxy - 2;
            curRow = std::min((int)lines.size() - 1, curRow + editorH);
            curCol = std::min(curCol, (int)lines[curRow].size());

        } else if (ch == KEY_BACKSPACE || ch == 127) {
            if (curCol > 0) {
                lines[curRow].erase(curCol - 1, 1);
                curCol--;
                dirty = true;
            } else if (curRow > 0) {
                curCol = (int)lines[curRow - 1].size();
                lines[curRow - 1] += lines[curRow];
                lines.erase(lines.begin() + curRow);
                curRow--;
                dirty = true;
            }

        } else if (ch == KEY_DC) {
            if (curCol < lineLen) {
                lines[curRow].erase(curCol, 1);
                dirty = true;
            } else if (curRow < (int)lines.size() - 1) {
                lines[curRow] += lines[curRow + 1];
                lines.erase(lines.begin() + curRow + 1);
                dirty = true;
            }

        } else if (ch == '\n' || ch == KEY_ENTER) {
            std::string rest = lines[curRow].substr(curCol);
            lines[curRow] = lines[curRow].substr(0, curCol);
            lines.insert(lines.begin() + curRow + 1, rest);
            curRow++; curCol = 0;
            dirty = true;

        } else if (ch == '\t') {
            lines[curRow].insert(curCol, "    ");
            curCol += 4;
            dirty = true;

        } else if (ch >= 32 && ch < 127) {
            lines[curRow].insert(curCol, 1, (char)ch);
            curCol++;
            dirty = true;
            statusMsg = "";
            statusIsError = false;
        }

        redraw();
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// MAIN
// ═════════════════════════════════════════════════════════════════════════════
int main(int argc, char* argv[]) {
    if (argc < 2) { std::cout << "\033[1;31mFatal Error:\033[0m No file specified\n"; return 1; }

    if (std::string(argv[1]) == "--version") {
        std::cout << ANSI_BOLD << "Nova Compiler" << ANSI_RESET 
          << " - Beta 2.0.1 (Build " << NOVA_BUILD_VERSION << ")\n";
        return 0;
    }
    if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        printHelp(); return 0;
    }

    std::vector<std::string> sourceFiles;
    std::string outputFile;
    int  optLevel   = 0;
    bool editorMode = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if      (arg == "-O2") { optLevel = 2; }
        else if (arg == "-O3") { optLevel = 3; }
        else if (arg == "-o") {
            if (i + 1 >= argc) { printError("'-o' requires an output filename"); return 1; }
            outputFile = argv[++i];
        } else if (arg == "-w") {
            editorMode = true;
        } else if (arg[0] == '-') {
            printError("unknown argument '" + arg + "'");
            return 1;
        } else {
            sourceFiles.push_back(arg);
        }
    }

    if (editorMode) {
        std::string editFile = sourceFiles.empty() ? "" : sourceFiles[0];
        runEditor(editFile, outputFile, optLevel);
        return 0;
    }

    if (sourceFiles.empty()) { printError("no source file specified"); return 1; }
    return compileFile(sourceFiles, outputFile, optLevel);
}
