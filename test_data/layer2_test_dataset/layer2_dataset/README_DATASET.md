# Layer 2 Test Dataset

Tổng số file: 171
Tổng số folder con được tạo: 30

Mục đích:
- test PUT/GET/DEL/LS/TREE
- test mapping path -> ordered chunk_ids
- test dedup với file trùng nội dung
- test file rỗng, file nhị phân, thư mục sâu, unicode path
- test rollback khi upload lỗi giữa chừng

Gợi ý test:
1. PUT toàn bộ thư mục theo từng file vào storage logic.
2. GET ngẫu nhiên vài file và so sánh SHA-256.
3. DEL các file duplicate rồi PUT lại để xem refCount/dedup.
4. LS ở các thư mục: docs/, data/, edge/, unicode/, deep/.
