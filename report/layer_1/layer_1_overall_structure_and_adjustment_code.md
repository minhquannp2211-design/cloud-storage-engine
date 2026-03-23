# Layer 1 module documents

## 1. `bloomfilter.pdf`
Mô tả module Bloom Filter theo hai góc nhìn:
- bản cũ dùng `key + "#" + n` để mô phỏng nhiều vị trí băm;
- bản đã vá dùng `compute_base_hashes()` và double hashing để giảm lặp tính toán.

Điểm chính của bản vá:
- thêm chặn cấu hình tối thiểu cho `bit_count` và `hash_count`;
- giữ nguyên vai trò “sàng lọc nhanh”, không thay thế `ChunkIndex`;
- thay đổi diễn ra trong lúc debug và tối ưu đường nóng dedup.

## 2. `chunker.pdf`
Mô tả lại đầy đủ module Chunker và nêu rõ điểm thay đổi quan trọng nhất:
- trạng thái cũ: đọc cả file vào RAM rồi mới chunk;
- trạng thái mới: đọc file theo block/chunk_size rồi sinh chunk dần.

Điểm chính của bản vá:
- thêm `reserve()` trong `chunk_bytes()`;
- đổi `chunk_file(path)` sang hướng buffered/block-based;
- cập nhật được thực hiện trong lúc debug, quan sát bottleneck ingest và tối ưu bộ nhớ.

## 3. `chunkindex.pdf`
Mô tả module chỉ mục chunk và persistence metadata.

Điểm chính của bản vá:
- tách `parse_index_line()`, `ordered_entries()`, `write_snapshot_file()`;
- `load_from_file()` chấp nhận store mới và fallback sang `.bak`;
- `save_to_file()` đổi từ ghi thẳng `trunc` sang ghi `temp + backup + rename`;
- thay đổi diễn ra trong lúc debug recovery, tối ưu persist và vá crash consistency tối thiểu.

## 4. `chunkstore.pdf`
Đây là tài liệu được cập nhật mạnh nhất vì module này thay đổi nhiều nhất sau khi sửa code.

Điểm chính của bản vá:
- bỏ mô hình flush index sau mỗi chunk;
- thêm `dirty_ops_`, `flush_threshold_`, `mark_dirty()`, `maybe_flush()`, `persist_if_dirty()`;
- `hash_chunk()` bỏ copy trung gian sang `std::string`;
- `write_chunk_file()` ghi qua file tạm;
- `gc()` chỉ xóa index entry khi xóa vật lý thành công hoặc file đã thật sự biến mất.

Việc thay đổi này diễn ra trong lúc debug deep runner, quan sát tốc độ ingest bất thường, sau đó tối ưu và vá bug correctness.

