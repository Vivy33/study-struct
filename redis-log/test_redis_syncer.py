import pytest
from unittest.mock import MagicMock, patch
from redis_syncer import RedisSyncer
import redis

# 测试 RedisSyncer 类的初始化
def test_redis_syncer_init():
    """
    测试 RedisSyncer 类的初始化方法。
    确保 Redis 客户端和日志文件路径被正确设置。
    """
    syncer = RedisSyncer()
    assert syncer.redis is not None  # 确保 Redis 客户端已初始化
    assert syncer.log_file == 'redis_log.log'  # 确保日志文件路径默认值正确

# 测试 set_key 方法成功设置键值对
def test_set_key_success():
    """
    测试 set_key 方法在成功设置键值对时的行为。
    使用 MagicMock 模拟 Redis 的 set 方法，并验证其被正确调用。
    """
    syncer = RedisSyncer()
    syncer.redis.set = MagicMock(return_value=True)  # 模拟 Redis set 方法成功
    syncer.set_key('test_key', 'test_value')
    syncer.redis.set.assert_called_once_with('test_key', 'test_value')  # 验证 set 方法被调用

# 测试 set_key 方法的重试逻辑
@patch('time.sleep', return_value=None)  # 模拟 time.sleep 方法
def test_set_key_retry(mock_sleep):
    """
    测试 set_key 方法在遇到连接错误时的重试逻辑。
    模拟 Redis 的 set 方法抛出 ConnectionError，并验证重试逻辑。
    """
    syncer = RedisSyncer()
    syncer.redis.set = MagicMock(side_effect=[redis.ConnectionError('Mocked connection error'), True])  # 模拟连接错误后成功
    syncer.set_key('test_key', 'test_value')  
    assert syncer.redis.set.call_count == 2  # 验证 set 方法被调用了两次
    mock_sleep.assert_called_once_with(5)  # 验证 time.sleep 被调用

# 测试 sync_logs 方法成功同步日志
def test_sync_logs_success(tmp_path):
    """
    测试 sync_logs 方法在成功同步日志时的行为。
    使用 tmp_path 创建临时日志文件，并验证 Redis 的 set 方法被正确调用。
    """
    log_file = tmp_path / 'test_log.log'  # 创建临时日志文件
    log_file.write_text("2023-10-10 12:00:00 - INFO - SET test_key test_value\n")  # 写入测试日志
    syncer = RedisSyncer(log_file=str(log_file))  # 初始化 RedisSyncer
    syncer.redis.set = MagicMock(return_value=True)  # 模拟 Redis set 方法成功
    syncer.sync_logs()  
    syncer.redis.set.assert_called_once_with('test_key', 'test_value')  # 验证 set 方法被调用

# 测试 sync_logs 方法对无效日志的处理
def test_sync_logs_invalid_log(tmp_path):
    """
    测试 sync_logs 方法对无效日志格式的处理。
    验证无效日志不会导致 Redis 操作。
    """
    log_file = tmp_path / 'test_log.log'  # 创建临时日志文件
    log_file.write_text("Invalid log format\n")  # 写入无效日志
    syncer = RedisSyncer(log_file=str(log_file))  # 初始化 RedisSyncer
    syncer.redis.set = MagicMock(return_value=True)  # 模拟 Redis set 方法成功
    syncer.sync_logs()  
    syncer.redis.set.assert_not_called()  # 验证 set 方法未被调用

# 测试 sync_logs 方法对不支持命令的处理
def test_sync_logs_unsupported_command(tmp_path):
    """
    测试 sync_logs 方法对不支持命令的处理。
    验证不支持的命令不会导致 Redis 操作。
    """
    log_file = tmp_path / 'test_log.log'  # 创建临时日志文件
    log_file.write_text("2023-10-10 12:00:00 - INFO - UNKNOWN test_key test_value\n")  # 写入不支持的命令
    syncer = RedisSyncer(log_file=str(log_file))  # 初始化 RedisSyncer
    syncer.redis.set = MagicMock(return_value=True)  # 模拟 Redis set 方法成功
    syncer.sync_logs()  
    syncer.redis.set.assert_not_called()  # 验证 set 方法未被调用

# 测试 sync_logs 方法对 GET 命令的处理
def test_sync_logs_get_command(tmp_path):
    """
    测试 sync_logs 方法对 GET 命令的处理。
    验证 GET 命令会调用 Redis 的 get 方法。
    """
    log_file = tmp_path / 'test_log.log'  # 创建临时日志文件
    log_file.write_text("2023-10-10 12:00:00 - INFO - GET test_key\n")  # 写入 GET 命令
    syncer = RedisSyncer(log_file=str(log_file))  # 初始化 RedisSyncer
    syncer.redis.get = MagicMock(return_value=b'test_value')  # 模拟 Redis get 方法成功
    syncer.sync_logs()  
    syncer.redis.get.assert_called_once_with('test_key')  # 验证 get 方法被调用

# 测试 sync_logs 方法对 DEL 命令的处理
def test_sync_logs_del_command(tmp_path):
    """
    测试 sync_logs 方法对 DEL 命令的处理。
    验证 DEL 命令会调用 Redis 的 delete 方法。
    """
    log_file = tmp_path / 'test_log.log'  # 创建临时日志文件
    log_file.write_text("2023-10-10 12:00:00 - INFO - DEL test_key\n")  # 写入 DEL 命令
    syncer = RedisSyncer(log_file=str(log_file))  # 初始化 RedisSyncer
    syncer.redis.delete = MagicMock(return_value=True)  # 模拟 Redis delete 方法成功
    syncer.sync_logs() 
    syncer.redis.delete.assert_called_once_with('test_key')  # 验证 delete 方法被调用

# 测试 sync_logs 方法在异常情况下的行为
def test_sync_logs_abort_on_error(tmp_path):
    """
    测试 sync_logs 方法在遇到异常时的行为。
    验证异常会被抛出，导致程序终止。
    """
    log_file = tmp_path / 'test_log.log'  # 创建临时日志文件
    log_file.write_text("2023-10-10 12:00:00 - INFO - SET test_key test_value\n")  # 写入测试日志
    syncer = RedisSyncer(log_file=str(log_file))  # 初始化 RedisSyncer
    syncer.redis.set = MagicMock(side_effect=redis.RedisError("Mocked Redis error"))  # 模拟 Redis 错误
    with pytest.raises(redis.RedisError):  # 验证异常是否被抛出
        syncer.sync_logs()  