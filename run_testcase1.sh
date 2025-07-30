#!/bin/bash

# 檢查 testcase1 目錄是否存在
if [ ! -d "testcase1" ]; then
    echo "錯誤：找不到 'testcase1' 目錄。"
    echo "請將此腳本放在 'testcase1' 的上一層目錄中執行。"
    exit 1
fi

# 設定基本變數
TESTCASE_DIR="testcase1"
EXECUTABLE="./build/src/openroad" # 請將 'cadb_0000_alpha' 換成您自己的執行檔名稱

# 檢查執行檔是否存在
if [ ! -f "$EXECUTABLE" ]; then
    echo "錯誤：找不到執行檔 '$EXECUTABLE'。"
    echo "請確認執行檔與此腳本位於同一目錄，或修改腳本中的 EXECUTABLE 變數。"
    exit 1
fi

# --- 自動尋找檔案 ---

# 1. 尋找所有 .lef 檔案
# 使用 find 命令在 testcase1 目錄下尋找所有 .lef 檔案
# xargs 將多行輸出轉換為單行，用空格分隔
LEF_FILES=$(find "$TESTCASE_DIR" -name "*.lef" | xargs)

# 2. 尋找所有 .lib 檔案
# 同樣地，尋找所有 .lib 檔案
LIB_FILES=$(find "$TESTCASE_DIR" -name "*.lib" | xargs)


# --- 組合並執行命令 ---

echo "找到的 LEF 檔案: $LEF_FILES"
echo "找到的 LIB 檔案: $LIB_FILES"
echo "---"
echo "準備執行..."
echo ""


# 將所有參數組合起來
# 使用 \ 來換行，讓命令更易讀
# -db 參數暫時用一個虛擬檔案 "dummy.db" 佔位，因為您的C++程式要求所有flag都必須存在
"$EXECUTABLE" \
    -weight "$TESTCASE_DIR/testcase1_weight" \
    -lib $LIB_FILES \
    -lef $LEF_FILES \
    -tf "$TESTCASE_DIR/testcase1.tf" \
    -sdc "$TESTCASE_DIR/testcase1.sdc" \
    -v "$TESTCASE_DIR/testcase1.v" \
    -def "$TESTCASE_DIR/testcase1.def" \
    -out "output_testcase1"


# 檢查上一個命令的返回狀態
if [ $? -eq 0 ]; then
    echo ""
    echo "---"
    echo "✅ 執行成功！"
    echo "輸出檔案前綴為 'output_testcase1'。"
else
    echo ""
    echo "---"
    echo "❌ 執行失敗。"
fi