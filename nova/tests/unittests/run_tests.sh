#!/bin/bash
# run_tests.sh — compila e roda todos os testes Nova
# Uso: ./run_tests.sh
# Requer: n++ no PATH

PASS=0
FAIL=0
RED='\033[1;31m'
GREEN='\033[1;32m'
CYAN='\033[1;36m'
BOLD='\033[1m'
RESET='\033[0m'

TESTS=(
    "01_primitivos.npp"
    "02_controle.npp"
    "03_funcoes.npp"
    "04_arrays.npp"
    "05_structs.npp"
    "06_globais.npp"
    "07_expressoes.npp"
)

EXPECTED=(
"13
7
30
3
1
5.140000
6.280000
Nova
-5"

"1
2
0
1
2
0
1
2
3
10
7
4
1
3
4
5"

"7
300
120
3628800
0
1
13
Hello Nova"

"10
30
50
2
11
5
28
2.500000
2
8"

"3
4
5
5
50
24
20
1
99"

"0
3
10
20
30
ok"

"14
20
1
2
0
15
12
24
6
6
4
1
1
1
1
1
0
16"
)

echo ""
echo -e "${BOLD}Nova Language — Test Suite${RESET}"
echo "─────────────────────────────────────"

for i in "${!TESTS[@]}"; do
    TEST="${TESTS[$i]}"
    EXP="${EXPECTED[$i]}"
    BIN="/tmp/nova_test_$i"

    # Compila
    COMPILE_OUT=$(n++ "$TEST" -o "$BIN" 2>&1)
    if [ $? -ne 0 ]; then
        echo -e "  ${RED}✗ FAIL${RESET}  $TEST"
        echo -e "         ${RED}compile error:${RESET} $COMPILE_OUT"
        FAIL=$((FAIL + 1))
        continue
    fi

    # Executa e captura output
    GOT=$("$BIN" 2>&1)
    rm -f "$BIN"

    if [ "$GOT" = "$EXP" ]; then
        echo -e "  ${GREEN}✓ PASS${RESET}  $TEST"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}✗ FAIL${RESET}  $TEST"
        echo -e "         ${CYAN}expected:${RESET}"
        echo "$EXP" | sed 's/^/           /'
        echo -e "         ${CYAN}got:${RESET}"
        echo "$GOT"  | sed 's/^/           /'
        FAIL=$((FAIL + 1))
    fi
done

echo "─────────────────────────────────────"
TOTAL=$((PASS + FAIL))
if [ $FAIL -eq 0 ]; then
    echo -e "  ${GREEN}${BOLD}All $TOTAL tests passed${RESET}"
else
    echo -e "  ${BOLD}$PASS/$TOTAL passed, ${RED}$FAIL failed${RESET}"
fi
echo ""