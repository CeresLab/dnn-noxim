
# Noxim  
NoC Simulation for Research Matrix Implementation  

<!-- ###################################################### -->  
<!-- #                    Instructions                    # -->  
<!-- ###################################################### -->  

---

## Installation Steps  

1. Clone DNN-Noxim from Github:

    ```bash
    git clone https://github.com/CeresLab/dnn-noxim.git
    ```

2. Enter the DNN-Noxim directory:

    ```bash
    cd dnn-noxim/
    ```

3. Run the `run.sh` script to install **SystemC**, **YAML**, and other related dependencies:

    ```bash
    bash run.sh
    ```

---

## File Preparation  

1. Navigate to the `bin` folder inside `dnn-noxim`:

    ```bash
    cd bin/
    ```

2. Prepare the `transaction_wb.txt` file following the required format.  
   Sample files (`transaction.txt` and `transaction_wb.txt`) are available in the `bin` folder for reference.

### File Format:  

- **First line:**  
  ```  
  #[src_type] [src] [dst_type] [dst]
  ```  
  - `src_type`: 0 for PE, 1 for Memory  
  - `src`: Source node ID  
  - `dst_type`: 0 for PE, 1 for Memory  
  - `dst`: Destination node ID  

- **Second line:**  
  ```  
  >? [operation_type] [activation_type]
  ```  
  - `operation_type`: 0 for Convolution, 1 for Fully connect, 2 for Pooling  
  - `activation_type`: 0 for No ReLU, 1 for ReLU  

- **Third line:**  
  ```  
  >! [0 < ctrl[0] <= 8] [0 < ctrl[1] <= 8] [ctrl[2]] [0 < ctrl[3] <= 8] [0 < ctrl[4] <= 8] [ctrl[5]]
  ```  
  - `kw`: Kernel width  
  - `kh`: Kernel height  
  - `stride`: Stride size  
  - `ifw`: Input width  
  - `ifh`: Input height  
  - `wb_dst`: Write-back destination node ID, or -1 if not required  

- **Fourth line:**  
  ```  
  >% [ifm: ctrl[3]*ctrl[4] data, w: ctrl[0]*ctrl[1] data]
  ```  
  - `ifm`: Input data (for example, if the input size is 4x4, provide 16 values)  
  - `w`: Kernel data (no data required for Pooling)

---

## Compilation Steps  

- **Note:** Recompilation is unnecessary if you only modify `transaction.txt` (or `transaction_wb.txt`) or YAML files.

1. Enter the `bin` folder:

    ```bash
    cd bin/
    ```

2. Compile the files:

    ```bash
    make -j12
    ```

---

## Simulation Execution  

- **Note:** Ensure the `default_config.yaml` file contains the following at the end:  
  ```yaml
  traffic_distribution: TRANSACTION_BASED
  traffic_table_filename: "transaction_wb.txt"
  ```

1. Enter the `bin` folder:

    ```bash
    cd bin/
    ```

2. Run the simulation:

    ```bash
    ./noxim -config ../config_examples/default_config.yaml > sim.out
    ```

3. Simulation results will be saved in the `sim.out` file located in the `bin` folder.

---

# 中文說明

---

## 安裝步驟

1. 從 Github 複製 DNN-Noxim：

    ```bash
    git clone https://github.com/CeresLab/dnn-noxim.git
    ```

2. 進入 DNN-Noxim 資料夾：

    ```bash
    cd dnn-noxim/
    ```

3. 執行 `run.sh` 來安裝 **SystemC**、**YAML** 等相關依賴：

    ```bash
    bash run.sh
    ```

---

## 準備所需檔案  

1. 進入 `dnn-noxim` 資料夾內的 `bin` 資料夾：

    ```bash
    cd bin/
    ```

2. 在 `bin` 資料夾中，依據格式準備 `transaction_wb.txt`。  
   `bin` 資料夾內已有範例檔案 (`transaction.txt` 及 `transaction_wb.txt`) 可供參考。

### 檔案格式：  

- **第一行：**  
  ```  
  [src_type] [src] [dst_type] [dst]
  ```  
  - `src_type`: 0 代表 PE，1 代表 Memory  
  - `src`: 源節點 ID  
  - `dst_type`: 0 代表 PE，1 代表 Memory  
  - `dst`: 目的節點 ID  

- **第二行：**  
  ```  
  >? [operation_type] [activation_type]
  ```  
  - `operation_type`: 0 代表 Convolution，1 代表 Fully connect，2 代表 Pooling  
  - `activation_type`: 0 代表不做 ReLU，1 代表做 ReLU  

- **第三行：**  
  ```  
  >! [0 < ctrl[0] <= 8] [0 < ctrl[1] <= 8] [ctrl[2]] [0 < ctrl[3] <= 8] [0 < ctrl[4] <= 8] [ctrl[5]]
  ```  
  - `kw`: Kernel 寬度  
  - `kh`: Kernel 長度  
  - `stride`: 一次移動的步數  
  - `ifw`: 輸入寬度  
  - `ifh`: 輸入長度  
  - `wb_dst`: 寫回目的地節點 ID，不需要寫回則設為 -1  

- **第四行：**  
  ```  
  >% [ifm: ctrl[3]*ctrl[4] data, w: ctrl[0]*ctrl[1] data]
  ```  
  - `ifm`: 輸入數據（例如輸入大小為 4x4 時，提供 16 個數值）  
  - `w`: Kernel 數據（Pooling 不需要數據）  

---

## 編譯步驟  

- **注意：** 若僅修改 `transaction.txt`（或 `transaction_wb.txt`）或 YAML 檔案，無需重新編譯。

1. 進入 `bin` 資料夾：

    ```bash
    cd bin/
    ```

2. 編譯檔案：

    ```bash
    make -j12
    ```

---

## 執行步驟  

- **注意：** 確保 `default_config.yaml` 檔案末尾包含以下內容：  
  ```yaml
  traffic_distribution: TRANSACTION_BASED
  traffic_table_filename: "transaction_wb.txt"
  ```

1. 進入 `bin` 資料夾：

    ```bash
    cd bin/
    ```

2. 執行模擬：

    ```bash
    ./noxim -config ../config_examples/default_config.yaml > sim.out
    ```

3. 模擬結果將存放於 `bin` 資料夾內的 `sim.out` 檔案中。
