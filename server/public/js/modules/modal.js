// 图片模态框模块
// 处理图片的弹窗显示、导航和关闭

import { imageUrls, currentImageIndex, setCurrentImageIndex } from './state.js';
import { parseImageDate } from './utils.js';

/**
 * 打开图片模态框
 * @param {string} imageUrl - 图片URL
 * @param {number} index - 图片在列表中的索引
 */
export function openImageModal(imageUrl, index = 0) {
    const modal = document.getElementById('imageModal');
    const modalImg = document.getElementById('modalImage');
    const caption = document.getElementById('caption');

    setCurrentImageIndex(index);
    modal.style.display = 'block';
    modalImg.src = imageUrl;
    caption.innerHTML = parseImageDate(imageUrl);

    updateNavigationButtons();
}

/**
 * 关闭图片模态框
 */
export function closeImageModal() {
    const modal = document.getElementById('imageModal');
    modal.style.display = 'none';
}

/**
 * 更新当前显示的图片
 * @param {number} newIndex - 新的图片索引
 */
function updateCurrentImage(newIndex) {
    if (imageUrls.length === 0 || newIndex < 0 || newIndex >= imageUrls.length) return;

    setCurrentImageIndex(newIndex);
    const imageUrl = imageUrls[currentImageIndex];

    const modalImg = document.getElementById('modalImage');
    const caption = document.getElementById('caption');

    modalImg.src = imageUrl;
    caption.innerHTML = parseImageDate(imageUrl);

    updateNavigationButtons();
}

/**
 * 显示上一张图片
 */
export function showPreviousImage() {
    if (imageUrls.length === 0 || currentImageIndex <= 0) return;
    updateCurrentImage(currentImageIndex - 1);
}

/**
 * 显示下一张图片
 */
export function showNextImage() {
    if (imageUrls.length === 0 || currentImageIndex >= imageUrls.length - 1) return;
    updateCurrentImage(currentImageIndex + 1);
}

/**
 * 更新导航按钮状态
 */
function updateNavigationButtons() {
    const prevBtn = document.getElementById('prevBtn');
    const nextBtn = document.getElementById('nextBtn');

    // 如果只有一张图片，隐藏导航按钮
    if (imageUrls.length <= 1) {
        prevBtn.style.display = 'none';
        nextBtn.style.display = 'none';
        return;
    }

    // 显示导航按钮
    prevBtn.style.display = 'block';
    nextBtn.style.display = 'block';

    // 设置按钮状态的通用函数
    function setButtonState(button, enabled) {
        if (enabled) {
            button.style.opacity = '1';
            button.style.cursor = 'pointer';
            button.style.pointerEvents = 'auto';
        } else {
            button.style.opacity = '0.3';
            button.style.cursor = 'not-allowed';
            button.style.pointerEvents = 'none';
        }
    }

    // 设置按钮启用/禁用状态
    setButtonState(prevBtn, currentImageIndex > 0);
    setButtonState(nextBtn, currentImageIndex < imageUrls.length - 1);
}

/**
 * 初始化图片模态框事件监听器
 */
export function initImageModal() {
    const modal = document.getElementById('imageModal');
    const closeBtn = document.querySelector('.close');
    const prevBtn = document.getElementById('prevBtn');
    const nextBtn = document.getElementById('nextBtn');

    // 点击关闭按钮关闭模态框
    closeBtn.onclick = closeImageModal;

    // 点击导航箭头
    prevBtn.onclick = showPreviousImage;
    nextBtn.onclick = showNextImage;

    // 点击模态框背景关闭模态框
    modal.onclick = function (event) {
        if (event.target === modal) {
            closeImageModal();
        }
    };

    // 按键导航
    document.addEventListener('keydown', function (event) {
        if (modal.style.display === 'block') {
            switch (event.key) {
                case 'Escape':
                    closeImageModal();
                    break;
                case 'ArrowLeft':
                    showPreviousImage();
                    break;
                case 'ArrowRight':
                    showNextImage();
                    break;
            }
        }
    });
}
