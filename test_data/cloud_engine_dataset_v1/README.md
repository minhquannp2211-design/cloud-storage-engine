# Cloud Engine Dataset v1

Dataset tổng hợp để kiểm tra Layer 1: chunking, hashing, dedup, refCount, read, delete, GC, recovery.

Nhóm dữ liệu:
- 01_exact_duplicates: file giống hệt nhau để test dedup cơ bản
- 02_near_duplicates_text: file text gần giống để test fixed-size vs CDC
- 03_near_duplicates_binary: binary gần giống nhau
- 04_edge_cases: file rỗng, file nhỏ, boundary quanh chunk size
- 05_random_high_entropy: dữ liệu entropy cao, dedup thấp
- 06_repetitive_logs: log/csv/json lặp nhiều để test dedup cao
- 07_nested_projects: cây thư mục sâu, nhiều file nhỏ
- 08_gc_delete_scenarios: workload manifest cho upload/delete/GC
- 09_recovery_checkpoints: manifest và snapshot giả lập cho basic recovery

Gợi ý chunk size để test: 4 KiB, 8 KiB, 64 KiB.
