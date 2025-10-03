-- CommentFree 数据库初始化脚本
-- 创建数据库和相关表结构

-- ================================================================
-- 1. 创建数据库（如果需要的话）
-- ================================================================
-- 注意：在运行此脚本之前，请确保已经创建了数据库
-- 可以使用命令：createdb commentfree

-- ================================================================
-- 2. 创建表结构
-- ================================================================

-- 创建评论主表
CREATE TABLE IF NOT EXISTS posts (
    id VARCHAR(16) PRIMARY KEY,                    -- 评论ID（如 word42）
    content TEXT NOT NULL,                         -- 评论内容
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- 创建时间
    view_count INTEGER DEFAULT 0,                 -- 浏览次数
    like_count INTEGER DEFAULT 0,                 -- 点赞次数
    ip_address INET,                               -- 发布者IP（可选，用于防护）
    user_agent TEXT                                -- 用户代理（可选）
);

-- 创建评论图片表
CREATE TABLE IF NOT EXISTS post_images (
    id SERIAL PRIMARY KEY,                         -- 自增主键
    post_id VARCHAR(16) NOT NULL,                  -- 关联评论ID
    path TEXT NOT NULL,                            -- 图片文件路径
    filename TEXT,                                 -- 原始文件名
    file_size INTEGER,                             -- 文件大小（字节）
    mime_type VARCHAR(100),                        -- MIME类型
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- 上传时间
    FOREIGN KEY (post_id) REFERENCES posts(id) ON DELETE CASCADE
);

-- ================================================================
-- 3. 创建索引（提高查询性能）
-- ================================================================

-- 评论表索引
CREATE INDEX IF NOT EXISTS idx_posts_created_at ON posts(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_posts_view_count ON posts(view_count DESC);
CREATE INDEX IF NOT EXISTS idx_posts_like_count ON posts(like_count DESC);

-- 图片表索引
CREATE INDEX IF NOT EXISTS idx_post_images_post_id ON post_images(post_id);
CREATE INDEX IF NOT EXISTS idx_post_images_created_at ON post_images(created_at DESC);

-- ================================================================
-- 4. 创建视图（便于查询）
-- ================================================================

-- 创建评论详情视图（包含图片数量）
CREATE OR REPLACE VIEW post_details AS
SELECT 
    p.id,
    p.content,
    p.created_at,
    p.view_count,
    p.like_count,
    p.ip_address,
    COALESCE(img_count.image_count, 0) as image_count
FROM posts p
LEFT JOIN (
    SELECT post_id, COUNT(*) as image_count
    FROM post_images
    GROUP BY post_id
) img_count ON p.id = img_count.post_id;

-- ================================================================
-- 5. 创建存储过程/函数（可选）
-- ================================================================

-- 增加浏览次数的函数
CREATE OR REPLACE FUNCTION increment_view_count(comment_id VARCHAR(16))
RETURNS BOOLEAN AS $$
BEGIN
    UPDATE posts 
    SET view_count = COALESCE(view_count, 0) + 1 
    WHERE id = comment_id;
    
    RETURN FOUND;
END;
$$ LANGUAGE plpgsql;

-- 增加点赞次数的函数
CREATE OR REPLACE FUNCTION increment_like_count(comment_id VARCHAR(16))
RETURNS BOOLEAN AS $$
BEGIN
    UPDATE posts 
    SET like_count = COALESCE(like_count, 0) + 1 
    WHERE id = comment_id;
    
    RETURN FOUND;
END;
$$ LANGUAGE plpgsql;

-- 获取评论统计信息的函数
CREATE OR REPLACE FUNCTION get_post_stats()
RETURNS TABLE(
    total_posts BIGINT,
    total_images BIGINT,
    total_views BIGINT,
    total_likes BIGINT,
    avg_content_length NUMERIC
) AS $$
BEGIN
    RETURN QUERY
    SELECT 
        COUNT(p.id) as total_posts,
        COUNT(pi.id) as total_images,
        SUM(COALESCE(p.view_count, 0)) as total_views,
        SUM(COALESCE(p.like_count, 0)) as total_likes,
        AVG(LENGTH(p.content)) as avg_content_length
    FROM posts p
    LEFT JOIN post_images pi ON p.id = pi.post_id;
END;
$$ LANGUAGE plpgsql;

-- ================================================================
-- 6. 插入示例数据（可选）
-- ================================================================

-- 插入示例评论（测试用）
INSERT INTO posts (id, content, view_count, like_count) VALUES 
('demo123', '这是一个示例评论，用于测试系统功能。这里有足够的文字来满足50字的最低要求。CommentFree让分享变得简单而美好！', 10, 3)
ON CONFLICT (id) DO NOTHING;

-- ================================================================
-- 7. 权限设置（根据需要调整）
-- ================================================================

-- 为应用用户创建专门的角色（可选）
-- CREATE ROLE commentfree_app LOGIN PASSWORD 'your_secure_password';
-- GRANT SELECT, INSERT, UPDATE ON posts TO commentfree_app;
-- GRANT SELECT, INSERT, UPDATE, DELETE ON post_images TO commentfree_app;
-- GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO commentfree_app;

-- ================================================================
-- 8. 数据库设置优化（可选）
-- ================================================================

-- 设置一些PostgreSQL参数优化
-- ALTER SYSTEM SET shared_preload_libraries = 'pg_stat_statements';
-- ALTER SYSTEM SET track_activity_query_size = 2048;
-- ALTER SYSTEM SET log_min_duration_statement = 1000;

-- ================================================================
-- 脚本执行完成信息
-- ================================================================

DO $$
BEGIN
    RAISE NOTICE '=================================================';
    RAISE NOTICE 'CommentFree 数据库初始化完成！';
    RAISE NOTICE '=================================================';
    RAISE NOTICE '已创建的表：';
    RAISE NOTICE '  - posts: 评论主表';
    RAISE NOTICE '  - post_images: 评论图片表';
    RAISE NOTICE '已创建的索引：';
    RAISE NOTICE '  - 时间索引、计数索引等';
    RAISE NOTICE '已创建的视图：';
    RAISE NOTICE '  - post_details: 评论详情视图';
    RAISE NOTICE '已创建的函数：';
    RAISE NOTICE '  - increment_view_count: 增加浏览次数';
    RAISE NOTICE '  - increment_like_count: 增加点赞次数';
    RAISE NOTICE '  - get_post_stats: 获取统计信息';
    RAISE NOTICE '=================================================';
    RAISE NOTICE '可以开始使用 CommentFree 系统了！';
    RAISE NOTICE '=================================================';
END $$;