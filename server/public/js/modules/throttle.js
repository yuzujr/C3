// throttle.js
// 节流工具函数，用于控制函数执行频率

/**
 * 节流函数
 * @param {Function} func - 要节流的函数
 * @param {number} delay - 延迟时间(毫秒)
 * @returns {Function} 节流后的函数
 */
export function throttle(func, delay) {
    let timeoutId;
    let lastExecTime = 0;
    
    return function (...args) {
        const currentTime = Date.now();
        
        if (currentTime - lastExecTime > delay) {
            // 如果距离上次执行已经超过delay时间，立即执行
            func.apply(this, args);
            lastExecTime = currentTime;
        } else {
            // 否则设置一个延迟执行
            clearTimeout(timeoutId);
            timeoutId = setTimeout(() => {
                func.apply(this, args);
                lastExecTime = Date.now();
            }, delay - (currentTime - lastExecTime));
        }
    };
}

/**
 * 防抖函数
 * @param {Function} func - 要防抖的函数
 * @param {number} delay - 延迟时间(毫秒)
 * @returns {Function} 防抖后的函数
 */
export function debounce(func, delay) {
    let timeoutId;
    
    return function (...args) {
        clearTimeout(timeoutId);
        timeoutId = setTimeout(() => func.apply(this, args), delay);
    };
}
