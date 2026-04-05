local redpacket_key = KEYS[1]
local redpacket_list = KEYS[2]
local total_amount = tonumber(ARGV[1])
local total_count = tonumber(ARGV[2])
local creator = ARGV[3]
local name = ARGV[4]
local redpacket_type = ARGV[5] or 'random'
local created_time = redis.call('TIME')[1]

-- 创建红包基本信息
redis.call('HSET', redpacket_key,
    'total_amount', total_amount,
    'total_count', total_count,
    'remain_amount', total_amount,
    'remain_count', total_count,
    'creator', creator,
    'created_time', created_time,
    'status', 1,
    'name', name,
    'type', redpacket_type
)

-- 设置过期时间(24小时)
redis.call('EXPIRE', redpacket_key, 86400)

-- 添加到红包列表
redis.call('ZADD', redpacket_list, created_time, string.sub(redpacket_key, 11))

return '1'