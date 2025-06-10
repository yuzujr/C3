// 日期解析工具模块
// 专门处理截图文件名中的日期解析

/**
 * 解析图片名中的日期
 * @param {string} imageUrl - 图片URL
 * @returns {string} 格式化的日期字符串或文件名
 */
export function parseImageDate(imageUrl) {
    const filename = imageUrl.split('/').pop();

    // 尝试匹配常见的日期时间格式
    // 格式如: screenshot_20240610_143025.jpg, 20240610-143025.png 等
    const patterns = [
        /(\d{4})(\d{2})(\d{2})[_-](\d{2})(\d{2})(\d{2})/,  // YYYYMMDD_HHMMSS 或 YYYYMMDD-HHMMSS
        /(\d{4})[_-](\d{2})[_-](\d{2})[_-](\d{2})[_-](\d{2})[_-](\d{2})/, // YYYY_MM_DD_HH_MM_SS
        /(\d{4})\.(\d{2})\.(\d{2})\.(\d{2})\.(\d{2})\.(\d{2})/, // YYYY.MM.DD.HH.MM.SS
    ];

    for (const pattern of patterns) {
        const match = filename.match(pattern);
        if (match) {
            const [, year, month, day, hour, minute, second] = match;
            const date = new Date(year, month - 1, day, hour, minute, second);

            // 格式化显示
            const options = {
                year: 'numeric',
                month: '2-digit',
                day: '2-digit',
                hour: '2-digit',
                minute: '2-digit',
                second: '2-digit',
                hour12: false
            };

            return date.toLocaleString('zh-CN', options);
        }
    }

    // 如果没有匹配到日期格式，返回文件名
    return filename;
}
