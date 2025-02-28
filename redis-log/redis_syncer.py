import redis
import logging
import time
from logging.handlers import RotatingFileHandler

class RedisSyncer:
    def __init__(self, host='localhost', port=6379, db=0, log_file='redis_log.log'):
        """
        初始化 RedisSyncer 类，配置 Redis 连接和日志记录。
        :param host: Redis 服务器地址，默认为 localhost
        :param port: Redis 服务器端口，默认为 6379
        :param db: Redis 数据库编号，默认为 0
        :param log_file: 日志文件路径，默认为 redis_log.log
        """
        self.redis = redis.Redis(host=host, port=port, db=db)
        self.log_file = log_file
        logging.basicConfig(level=logging.INFO,
                            handlers=[RotatingFileHandler(log_file, maxBytes=10*1024*1024, backupCount=5)],
                            format='%(asctime)s - %(levelname)s - %(message)s')

    def set_key(self, key, value):
        """
        在 Redis 中设置键值对，并记录操作到日志。
        :param key: Redis 键
        :param value: Redis 值
        """
        try:
            self.redis.set(key, value)
            logging.info(f"SET {key} {value}")
        except redis.ConnectionError as e:
            logging.error(f"Connection error setting key {key}: {e}")
            time.sleep(5)
            self.set_key(key, value)
        except Exception as e:
            logging.error(f"Error setting key {key}: {e}")
            raise

    def sync_logs(self):
        """
        从日志文件中读取操作记录，并同步到 Redis。
        """
        with open(self.log_file, 'r') as f:
            for line in f:
                parts = line.split(' - ')[1].strip().split(' ')
                if len(parts) < 3:
                    logging.warning(f"Invalid log format: {line.strip()}")
                    continue
                command = parts[0]
                if command == 'SET':
                    key = parts[1]
                    value = parts[2]
                    self.redis.set(key, value)
                elif command == 'GET':
                    key = parts[1]
                    value = self.redis.get(key)
                    logging.info(f"Synced GET command: {line.strip()} - Value: {value}")
                elif command == 'DEL':
                    key = parts[1]
                    self.redis.delete(key)
                    logging.info(f"Synced DEL command: {line.strip()}")
                else:
                    logging.warning(f"Unsupported command: {command}")

if __name__ == "__main__":
    syncer = RedisSyncer()
    syncer.set_key('test_key', 'test_value')
    syncer.sync_logs()