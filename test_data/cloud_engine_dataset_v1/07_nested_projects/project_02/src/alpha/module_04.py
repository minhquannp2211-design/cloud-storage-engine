# project 2, module 4
def chunk_id(data):
    return 'sha256:' + str(len(data))

class RefCounter:
    def __init__(self):
        self.count = 0
    def inc(self):
        self.count += 1
    def dec(self):
        self.count -= 1
