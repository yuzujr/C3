// 删除功能模块
// 处理截图的删除操作

import { selectedClient } from './state.js';
import { fetchScreenshots } from './screenshots.js';
import { showToast } from '../../toast/toast.js';

/**
 * 删除指定时间范围内的截图
 * @param {number} hours - 删除多少小时前的截图
 */
export async function deleteScreenshots(hours) {
    if (!selectedClient) {
        showToast('请先选择客户端');
        return;
    }

    try {
        const response = await fetch(`/web/delete-screenshots/${selectedClient}`, {
            method: 'DELETE',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                hours: hours
            })
        });

        if (response.ok) {
            const result = await response.json();
            showToast(`删除成功：已删除 ${result.deletedCount || 0} 个文件`);

            // 关闭对话框
            document.getElementById('deleteModal').style.display = 'none';

            // 重新加载截图
            await fetchScreenshots(selectedClient);
        } else {
            const error = await response.text();
            showToast(`删除失败：${error}`);
        }
    } catch (error) {
        console.error('删除截图时出错:', error);
        showToast('删除失败：网络错误');
    }
}

/**
 * 删除所有截图
 */
export async function deleteAllScreenshots() {
    if (!selectedClient) {
        showToast('请先选择客户端');
        return;
    }

    try {
        const response = await fetch(`/web/delete-all-screenshots/${selectedClient}`, {
            method: 'DELETE',
            headers: {
                'Content-Type': 'application/json'
            }
        });

        if (response.ok) {
            const result = await response.json();
            showToast(`删除成功：已删除 ${result.deletedCount || 0} 个文件`);

            // 关闭对话框
            document.getElementById('deleteModal').style.display = 'none';

            // 重新加载截图
            await fetchScreenshots(selectedClient);
        } else {
            const error = await response.text();
            showToast(`删除失败：${error}`);
        }
    } catch (error) {
        console.error('删除所有截图时出错:', error);
        showToast('删除失败：网络错误');
    }
}

/**
 * 初始化删除截图功能
 */
export function initDeleteScreenshots() {
    const deleteBtn = document.getElementById('deleteScreenshotsBtn');
    const deleteModal = document.getElementById('deleteModal');
    const cancelBtn = document.getElementById('cancelDelete');
    const delete1HourBtn = document.getElementById('delete1Hour');
    const delete1DayBtn = document.getElementById('delete1Day');
    const delete1WeekBtn = document.getElementById('delete1Week');
    const deleteAllBtn = document.getElementById('deleteAll');

    // 打开删除对话框
    deleteBtn.onclick = () => {
        if (!selectedClient) {
            showToast('请先选择客户端');
            return;
        }
        deleteModal.style.display = 'block';
    };

    // 取消删除
    cancelBtn.onclick = () => {
        deleteModal.style.display = 'none';
    };

    // 点击模态框背景关闭
    deleteModal.onclick = function (event) {
        if (event.target === deleteModal) {
            deleteModal.style.display = 'none';
        }
    };

    // 删除不同时间范围的截图
    delete1HourBtn.onclick = () => deleteScreenshots(1);
    delete1DayBtn.onclick = () => deleteScreenshots(24);
    delete1WeekBtn.onclick = () => deleteScreenshots(24 * 7);
    deleteAllBtn.onclick = () => deleteAllScreenshots();
}
