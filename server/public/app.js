let selectedClient = null;
let refreshInterval = null;
const CLIENT_POLL_INTERVAL = 3000; // 3秒
const SCREENSHOTS_POLL_INTERVAL = 3000; // 3秒

async function fetchClients() {
  const res = await fetch('/web/clients');
  const clients = await res.json();
  const clientsList = document.getElementById('clientsList');
  clientsList.innerHTML = '';
  if (clients.length === 0) {
    clientsList.textContent = '无客户端在线';
    selectedClient = null;
    document.getElementById('commands').style.display = 'none';
    document.getElementById('screenshots').textContent = '请选择客户端';
    return;
  }

  clients.forEach(clientId => {
    const div = document.createElement('div');
    div.textContent = clientId;
    div.className = 'client-item';
    if (clientId === selectedClient) {
      div.classList.add('selected');
    }
    div.onclick = () => selectClient(clientId);
    clientsList.appendChild(div);
  });
}

async function selectClient(clientId) {
  selectedClient = clientId;
  updateClientHighlight();

  document.getElementById('screenshots').textContent = '加载截图中...';
  document.getElementById('cmdButtons').style.display = 'block';

  let lastScreenshotTime = await fetchScreenshots(clientId);
  await loadClientConfig(clientId);
  
  if (refreshInterval) clearInterval(refreshInterval);
  
  // 使用自执行异步函数来处理轮询
  refreshInterval = setInterval(async () => {
    try {
      lastScreenshotTime = await fetchNewScreenshots(selectedClient, lastScreenshotTime);
    } catch (err) {
      console.error('获取新截图失败:', err);
      // 即使失败也更新时间戳，避免重复请求失败的内容
      lastScreenshotTime = Date.now();
    }
  }, SCREENSHOTS_POLL_INTERVAL);
}

function updateClientHighlight() {
  document.querySelectorAll('.client-item').forEach(div => {
    if (div.textContent === selectedClient) {
      div.classList.add('selected');
    } else {
      div.classList.remove('selected');
    }
  });
}

async function fetchScreenshots(clientId) {
  const res = await fetch(`/web/screenshots/${clientId}`);
  const screenshots = await res.json();
  const container = document.getElementById('screenshots');
  container.innerHTML = '';
  if (screenshots.length === 0) {
    container.textContent = '该客户端暂无截图';
    return;
  }
  screenshots.forEach(url => {
    const img = document.createElement('img');
    img.src = url;
    container.appendChild(img);
  });

  return Date.now(); // 返回当前时间戳用于下次请求
}

async function fetchNewScreenshots(clientId, lastTimestamp) {
  // 确保 lastTimestamp 是数字类型
  const timestamp = typeof lastTimestamp === 'number' ? lastTimestamp : 
                    (lastTimestamp instanceof Promise ? 0 : parseInt(lastTimestamp) || 0);
  
  const res = await fetch(`/web/screenshots/${clientId}?since=${timestamp}`);
  const screenshots = await res.json();
  if (screenshots.length != 0) {
        console.log('新截图:', screenshots);
  }
  
  if (screenshots.length > 0) {
    const container = document.getElementById('screenshots');
    screenshots.forEach(url => {
      const img = document.createElement('img');
      img.src = url;
      container.insertBefore(img, container.firstChild);
    });
  }
  
  return Date.now(); // 返回当前时间戳用于下次请求
}

async function sendCommand(command) {
  if (!selectedClient) {
    showToast('请先选择客户端');
    return;
  }
  const res = await fetch('/web/send_commands', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({
      client_id: selectedClient,
      command: command
    })
  });
  showToast(res.ok ? '命令发送成功' : '命令发送失败');
}

async function clearCommands() {
    if (!selectedClient) {
      showToast('请先选择客户端');
      return;
    }
    
    const res = await fetch('/web/clear_commands', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({
        client_id: selectedClient
      })
    });
    
    showToast(res.ok ? '命令队列已清空' : '清除命令队列失败');
  }

document.getElementById('commands').addEventListener('click', e => {
  if (e.target.tagName === 'BUTTON' && e.target.dataset.cmd) {
    const cmd = JSON.parse(e.target.dataset.cmd);
    sendCommand(cmd);
  }
});
document.getElementById('clearCommandsBtn').addEventListener('click', clearCommands);

document.getElementById('updateConfigBtn').onclick = async () => {
  if (!selectedClient) {
    showToast('请先选择客户端');
    return;
  }

  const serverUrl = document.getElementById('serverUrl').value;
  const intervalSeconds = parseInt(document.getElementById('intervalSeconds').value);
  const maxRetries = parseInt(document.getElementById('maxRetries').value);
  const retryDelayMs = parseInt(document.getElementById('retryDelayMs').value);
  const addToStartup = document.getElementById('addToStartup').checked;

  if (!serverUrl || isNaN(intervalSeconds) || isNaN(maxRetries) || isNaN(retryDelayMs)) {
    showToast('请填写正确的配置参数');
    return;
  }

  const newConfig = {
    api: {
      server_url: serverUrl,
      interval_seconds: intervalSeconds,
      max_retries: maxRetries,
      retry_delay_ms: retryDelayMs,
      add_to_startup: addToStartup
    }
  };

  const cmd = {
    type: "update_config",
    data: newConfig
  };
  await sendCommand(cmd);
};

async function loadClientConfig(clientId) {
    try {
      const res = await fetch(`/web/config/${clientId}`);
      if (!res.ok) {
        console.warn('获取配置失败');
        console.warn(await res.text());
        return;
      }
      const config = await res.json();
  
      document.getElementById('serverUrl').value = config.server_url || '';
      document.getElementById('intervalSeconds').value = config.interval_seconds ?? '';
      document.getElementById('maxRetries').value = config.max_retries ?? '';
      document.getElementById('retryDelayMs').value = config.retry_delay_ms ?? '';
      document.getElementById('addToStartup').checked = !!config.add_to_startup;
    } catch (err) {
      console.error('加载客户端配置出错:', err);
    }
  }

window.onload = () => {
  fetchClients();
  setInterval(fetchClients, CLIENT_POLL_INTERVAL);

  document.getElementById('cmdButtons').style.display = 'none';
};
