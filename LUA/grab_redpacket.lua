local redpacket_key = KEYS[1]
local user_grabbed_key = KEYS[2]
local user_grab_key = KEYS[3]
local user_id = ARGV[1]
local redpacket_id = ARGV[2]

-- 检查红包状态
local redpacket_info = redis.call('HMGET', redpacket_key, 'status', 'remain_count', 'remain_amount', 'type')
local status = redpacket_info[1]
local remain_count = tonumber(redpacket_info[2])
local remain_amount = tonumber(redpacket_info[3])
local redpacket_type = redpacket_info[4]

if not status or status == '0' or status == '-1' then
    return '0'
end

if remain_count <= 0 then
    redis.call('HSET', redpacket_key, 'status', '0')
    return '0'
end

-- 检查用户是否已抢过
local has_grabbed = redis.call('SISMEMBER', user_grabbed_key, redpacket_id)
if has_grabbed == 1 then
    return '-1'
end

-- 计算抢到的金额
local grab_amount = 0
if redpacket_type == 'fixed' then
    grab_amount = math.floor(remain_amount / remain_count)
else
    if remain_count == 1 then
        grab_amount = remain_amount
    else
        local max_amount = math.floor(remain_amount / remain_count * 2)
        grab_amount = math.random(1, max_amount)
        if grab_amount > remain_amount then
            grab_amount = remain_amount
        end
    end
end

-- 更新红包信息
local new_remain_amount = remain_amount - grab_amount
local new_remain_count = remain_count - 1

redis.call('HSET', redpacket_key, 
    'remain_amount', new_remain_amount,
    'remain_count', new_remain_count
)

if new_remain_count == 0 then
    redis.call('HSET', redpacket_key, 'status', '0')
end

-- 记录用户抢红包结果
redis.call('HSET', user_grab_key, user_id, grab_amount)
redis.call('SADD', user_grabbed_key, redpacket_id)

return tostring(grab_amount)