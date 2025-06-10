// 状态管理模块
// 全局状态变量的集中管理

export let selectedClient = null;
export let currentImageIndex = 0;
export let imageUrls = [];
export let webSocket = null;

// 常量配置
export const CLIENT_POLL_INTERVAL = 1000; // 1秒 - 客户端列表更新频率

// 状态更新函数
export function setSelectedClient(clientId) {
    selectedClient = clientId;
}

export function setCurrentImageIndex(index) {
    currentImageIndex = index;
}

export function setImageUrls(urls) {
    imageUrls = [...urls];
}

export function setWebSocket(ws) {
    webSocket = ws;
}

export function addNewImageUrl(url) {
    if (!imageUrls.includes(url)) {
        imageUrls.unshift(url);
    }
}

export function clearImageUrls() {
    imageUrls = [];
}
