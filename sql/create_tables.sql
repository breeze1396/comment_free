-- CommentFree 基础数据库初始化脚本
-- 仅包含必要的表结构，用于快速部署

-- 创建评论主表
CREATE TABLE IF NOT EXISTS posts (
    id VARCHAR(16) PRIMARY KEY,
    content TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT NOW(),
    view_count INTEGER DEFAULT 0,
    like_count INTEGER DEFAULT 0
);

-- 创建评论图片表  
CREATE TABLE IF NOT EXISTS post_images (
    id SERIAL PRIMARY KEY,
    post_id VARCHAR(16) REFERENCES posts(id) ON DELETE CASCADE,
    path TEXT NOT NULL
);

-- 创建基本索引
CREATE INDEX IF NOT EXISTS idx_posts_created_at ON posts(created_at);
CREATE INDEX IF NOT EXISTS idx_post_images_post_id ON post_images(post_id);

-- 插入测试数据
INSERT INTO posts (id, content, view_count, like_count) VALUES 
('test001', '这是一个测试评论，用来验证系统是否正常工作。CommentFree系统支持文字和图片的分享，让每个人都能自由表达自己的想法和创意。', 5, 2)
ON CONFLICT (id) DO NOTHING;