# Noxim
NoC simulation for research matrix implementation

<!-- ###################################################### -->
<!-- #                    Instruction                     # -->
<!-- ###################################################### -->


############################ Installation Steps Start ############################

# Clone DNN-Noxim from Github

    git clone https://github.com/CeresLab/dnn-noxim.git

# Enter the dnn-noxim folder

    cd dnn-noxim/

# Run run.sh found in the path to install SystemC, YAML, and other related files

    bash run.sh



######################## Preparation of Required Files Steps Start ########################

# Enter the bin folder inside dnn-noxim

    cd bin/

# In the bin folder, prepare transaction_wb.txt with the required format
# A sample file is already available in the bin folder for reference (transaction.txt and transaction_wb.txt)

# First line 
# >[src_type] [src] [dst_type] [dst]

src_type : 0 means the source node is PE, 1 means the source node is Memory
src : Source node ID
dst_type : 0 means the destination node is PE, 1 means the destination node is Memory
dst : Destination node ID

# Second line
# >? [operation_type] [activation_type]

operation_type : 0 means Convolution, 1 means Fully connect, 2 means Pooling
activation_type : 0 means no ReLU, 1 means ReLU

# Third line
# >! [0 < ctrl[0] <= 8] [0 < ctrl[1] <= 8] [ctrl[2]] [0 < ctrl[3] <= 8] [0 < ctrl[4] <= 8] [ctrl[5]]

kw : Kernel width
kh : Kernel height
stride : Kernel stride
ifw : Input width
ifh : Input height
wb_dst : Write-back destination node ID, set to -1 if no write-back is needed

# Fourth line
# >% [ifm: ctrl[3]*ctrl[4] data, w: ctrl[0]*ctrl[1] data]

ifm : Input values (for example, if the input size is 4x4, provide 16 values)
w : Kernel values (no values needed for Pooling)

######################## Compilation Steps Start ########################

# Note: If only modifying transaction.txt (transaction_wb.txt) or YAML files, recompilation is not necessary

# Enter the bin folder inside dnn-noxim

# Compile the files

    make -j12



######################## Execution Steps Start ########################

# Note: Ensure the last lines in the default_config.yaml file are
# traffic_distribution: TRANSACTION_BASED and traffic_table_filename: "transaction_wb.txt"

# Enter the bin folder inside dnn-noxim

# Run the simulation

    ./noxim -config ../config_examples/default_config.yaml > sim.out

# The simulation result file will be placed in the bin folder under the name sim.out



<!-- ###################################################### -->
<!-- #                      中文說明                       # -->
<!-- ###################################################### -->

############################ 安裝步驟開始 ############################

# 從Github複製DNN-Noxim

    git clone https://github.com/CeresLab/dnn-noxim.git

# 進入dnn-noxim資料夾

    cd dnn-noxim/

# 執行在路徑中找到的 run.sh，安裝SystemC 與 YAML等相關檔案 

    bash run.sh



######################## 準備所需檔案步驟開始 ########################

# 進入dnn-noxim內bin資料夾

    cd bin/

# 在bin資料夾準備transaction_wb.txt需定義之格式
# bin資料夾內已有範例檔案可參考(transaction.txt 以及transaction_wb.txt)

# 第一行 >
# [src_type] [src] [dst_type] [dst]

src_type : 0代表源節點為PE 1代表源節點為Memory
src : 源節點ID
dst_type : 0代表目的地節點為PE 1代表目的地節點為Memory
dst : 目的地節點ID

# 第二行
# >? [operation_type] [activation_type]

opteration_type : 0代表做Convolution 1代表做Fully connect 2代表做Pooling
activation_type : 0代表不做ReLU 1代表做ReLU

# 第三行
# >! [0 < ctrl[0] <= 8] [0 < ctrl[1] <= 8] [ctrl[2]] [0 < ctrl[3] <= 8] [0 < ctrl[4] <= 8] [ctrl[5]]

kw : Kernel寬度
kh : Kernel長度
stride : Kernel一次移動的跨步
ifw : 輸入寬度
ifh : 輸入長度
wb_dst : 寫回目的地節點的ID 如不需寫回則設為-1 

# 第四行
# >% [ifm: ctrl[3]*ctrl[4] data, w: ctrl[0]*ctrl[1] data]

ifm : 輸入值(如為4x4輸入大小則需給定16個值)
w : kernel值(如為Pooling則不需給值)



######################## 編譯步驟開始 ########################

# 注意 : 如只修改transaction.txt(transaction_wb.txt)或yaml檔案不需重新編譯

# 進入dnn-noxim內bin資料夾

# 編譯檔案
    make -j12



######################## 執行步驟開始 ########################

# 注意 : 確保default_config.yaml 檔案內最後為
# traffic_distribution: TRANSACTION_BASED 以及 traffic_table_filename:"transaction_wb.txt"

# 進入dnn-noxim內bin資料夾

# 執行模擬
    ./noxim -config ../config_examples/default_config.yaml > sim.out

# 模擬結果檔案放置在bin資料夾內的sim.out檔案




