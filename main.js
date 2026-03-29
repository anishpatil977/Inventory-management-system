// ==================== UTILITY FUNCTIONS ====================
async function api(path, opts = {}) {
    try {
        const res = await fetch(path, opts);
        const txt = await res.text();
        try {
            return { status: res.status, body: JSON.parse(txt) };
        } catch (e) {
            return { status: res.status, body: txt };
        }
    } catch (err) {
        console.error('API error:', err);
        return { status: 0, body: null, error: err.message };
    }
}

function finalPrice(base, disc, tax) {
    const net = base * (1 - disc);
    return net * (1 + tax);
}

function escapeHtml(s) {
    return (s + '')
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;');
}

// ==================== PAGE NAVIGATION ====================
function initPageNavigation() {
    const navTabs = document.querySelectorAll('.nav-tab');
    if (!navTabs || navTabs.length === 0) {
        console.warn('No navigation tabs found');
        return;
    }

    navTabs.forEach(tab => {
        tab.addEventListener('click', () => {
            const pageName = tab.getAttribute('data-page');
            if (!pageName) return;

            switchPage(pageName);

            // Update active tab styling
            navTabs.forEach(t => t.classList.remove('active'));
            tab.classList.add('active');
        });
    });
}

function switchPage(pageName) {
    const pages = document.querySelectorAll('.page');
    if (!pages || pages.length === 0) {
        console.warn('No pages found');
        return;
    }

    pages.forEach(page => page.classList.remove('active'));

    const targetPage = document.getElementById(pageName);
    if (targetPage) {
        targetPage.classList.add('active');

        // Refresh orders data when switching to orders page
        if (pageName === 'orders') {
            renderPendingOrders();
        }
    }
}

// ==================== INVENTORY FUNCTIONS ====================
function render(items) {
    const tbody = document.querySelector('#tbl tbody');
    if (!tbody) return;

    tbody.innerHTML = '';

    if (!items || items.length === 0) {
        const tr = document.createElement('tr');
        tr.innerHTML = '<td colspan="6" style="text-align: center; color: #999;">No products found</td>';
        tbody.appendChild(tr);
        return;
    }

    for (const it of items) {
        const tr = document.createElement('tr');
        const final = finalPrice(
            Number(it.base) || 0,
            Number(it.discount) || 0,
            Number(it.tax) || 0
        ).toFixed(2);

        tr.innerHTML = `
            <td>${escapeHtml(it.id)}</td>
            <td>${escapeHtml(it.name)}</td>
            <td>$${Number(it.base || 0).toFixed(2)}</td>
            <td>${(Number(it.tax || 0) * 100).toFixed(0)}%</td>
            <td>${(Number(it.discount || 0) * 100).toFixed(0)}%</td>
            <td><strong>$${final}</strong></td>
        `;
        tbody.appendChild(tr);
    }
}

const STORAGE_KEY = 'inventory-data-v1';

function loadInventory() {
    try {
        const raw = localStorage.getItem(STORAGE_KEY);
        return raw ? JSON.parse(raw) : [];
    } catch (err) {
        console.warn('Failed to parse inventory from localStorage', err);
        return [];
    }
}

function saveInventory(items) {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(items));
}

function listAll() {
    render(loadInventory());
}

function search(q) {
    if (!q) {
        listAll();
        return;
    }

    const filtered = loadInventory().filter(x => x.name.toLowerCase().includes(q.toLowerCase()));
    render(filtered);
}

function addItem() {
    const id = document.getElementById('id').value.trim();
    const name = document.getElementById('name').value.trim();
    const base = Number(document.getElementById('base').value.trim() || '0');
    const tax = Number(document.getElementById('tax').value.trim() || '0');
    const discount = Number(document.getElementById('discount').value.trim() || '0');

    if (!id || !name) {
        showMsg('Product ID and Name are required', true);
        return;
    }

    if (isNaN(base) || isNaN(tax) || isNaN(discount)) {
        showMsg('Base price, tax, and discount must be numbers', true);
        return;
    }

    if (tax < 0 || tax > 1 || discount < 0 || discount > 1) {
        showMsg('Tax and discount must be between 0 and 1', true);
        return;
    }

    const items = loadInventory();
    if (items.some(item => item.id === id)) {
        showMsg('Product ID already exists', true);
        return;
    }

    items.push({ id, name, base, tax, discount });
    saveInventory(items);

    showMsg('✓ Product added successfully');
    clearInputs();
    listAll();
}

function removeItem() {
    const id = document.getElementById('id').value.trim();
    if (!id) {
        showMsg('Enter Product ID to remove', true);
        return;
    }

    let items = loadInventory();
    const updated = items.filter(item => item.id !== id);

    if (updated.length === items.length) {
        showMsg('Product ID not found', true);
        return;
    }

    saveInventory(updated);
    showMsg('✓ Product removed successfully');
    clearInputs();
    listAll();
}

function showMsg(s, isErr = false) {
    const el = document.getElementById('msg');
    if (!el) return;

    el.className = isErr ? 'error' : 'success';
    el.textContent = s;

    setTimeout(() => {
        if (el) {
            el.textContent = '';
            el.className = '';
        }
    }, 3000);
}

function clearInputs() {
    ['id', 'name', 'base', 'tax', 'discount'].forEach(id => {
        const el = document.getElementById(id);
        if (el) el.value = '';
    });
}

// ==================== PENDING ORDERS ====================
const pendingOrdersData = [
    { id: 'ORD-001', customer: 'John Smith', items: 'Laptop x1, Mouse x2', total: '$1,299.99', status: 'pending', date: '2024-03-29' },
    { id: 'ORD-002', customer: 'Jane Doe', items: 'Monitor x1, Keyboard x1', total: '$589.99', status: 'processing', date: '2024-03-28' },
    { id: 'ORD-003', customer: 'Bob Johnson', items: 'USB-C Cable x5', total: '$65.99', status: 'pending', date: '2024-03-29' },
    { id: 'ORD-004', customer: 'Alice Brown', items: 'Desk Chair x1', total: '$349.99', status: 'processing', date: '2024-03-27' },
    { id: 'ORD-005', customer: 'Charlie Wilson', items: 'Webcam x1, Microphone x1', total: '$189.99', status: 'pending', date: '2024-03-29' },
    { id: 'ORD-006', customer: 'Diana Lee', items: 'Laptop Stand x1, Monitor x1', total: '$449.99', status: 'completed', date: '2024-03-25' }
];

function renderPendingOrders() {
    const grid = document.getElementById('ordersGrid');
    if (!grid) return;

    grid.innerHTML = '';

    if (!pendingOrdersData || pendingOrdersData.length === 0) {
        grid.innerHTML = '<p style="grid-column: 1/-1; text-align: center; color: #999; padding: 40px;">No pending orders</p>';
        return;
    }

    pendingOrdersData.forEach(order => {
        const card = document.createElement('div');
        card.className = 'order-card';

        const statusClass = `status-${order.status}`;
        const statusLabel = (order.status.charAt(0).toUpperCase() + order.status.slice(1));

        card.innerHTML = `
            <h3>${escapeHtml(order.id)}</h3>
            <p><strong>👤 Customer:</strong> ${escapeHtml(order.customer)}</p>
            <p><strong>📦 Items:</strong> ${escapeHtml(order.items)}</p>
            <p><strong>💰 Total:</strong> ${order.total}</p>
            <p><strong>📅 Date:</strong> ${order.date}</p>
            <span class="order-status ${statusClass}">${statusLabel}</span>
        `;

        grid.appendChild(card);
    });
}

// ==================== BACKGROUND WIDGET ====================
const panel = document.getElementById('bgPanel');
const toggleBtn = document.getElementById('bgWidgetButton');
const bgUrlInput = document.getElementById('bgUrl');

function setBackground(url) {
    if (!url || typeof url !== 'string') {
        showMsg('Invalid background URL', true);
        return;
    }

    try {
        document.body.style.backgroundImage = `url('${url}')`;
        document.body.style.backgroundSize = 'cover';
        document.body.style.backgroundRepeat = 'no-repeat';
        document.body.style.backgroundPosition = 'center center';
        document.body.style.backgroundAttachment = 'fixed';
        showMsg('✓ Background updated');
    } catch (err) {
        console.error('Background error:', err);
        showMsg('Failed to set background', true);
    }
}

function applyPreset(url) {
    if (bgUrlInput) bgUrlInput.value = url;
    setBackground(url);
}

function resetBackground() {
    document.body.style.backgroundImage = '';
    document.body.style.background = 'linear-gradient(135deg, #667eea 0%, #764ba2 100%)';
    if (bgUrlInput) bgUrlInput.value = '';
    showMsg('✓ Background reset to default');
}

function toggleBgPanel() {
    if (!panel) return;

    const isOpen = panel.classList.contains('show');
    if (isOpen) {
        panel.classList.remove('show');
        if (toggleBtn) toggleBtn.textContent = '🖼️';
    } else {
        panel.classList.add('show');
        if (toggleBtn) toggleBtn.textContent = '✕';
    }
}

// Setup background widget event listeners
if (toggleBtn) {
    toggleBtn.onclick = toggleBgPanel;
}

const btnSetBg = document.getElementById('btnSetBg');
if (btnSetBg) {
    btnSetBg.onclick = () => {
        const url = bgUrlInput ? bgUrlInput.value.trim() : '';
        if (!url) {
            showMsg('Paste a background image URL (https://...)', true);
            return;
        }
        setBackground(url);
    };
}

const btnResetBg = document.getElementById('btnResetBg');
if (btnResetBg) {
    btnResetBg.onclick = resetBackground;
}

// Close panel when clicking outside
document.addEventListener('click', (e) => {
    if (panel && toggleBtn && !document.getElementById('bgWidget')?.contains(e.target)) {
        if (panel.classList.contains('show')) {
            panel.classList.remove('show');
            toggleBtn.textContent = '🖼️';
        }
    }
});

// ==================== EVENT LISTENERS ====================
const btnRefresh = document.getElementById('btnRefresh');
if (btnRefresh) btnRefresh.onclick = listAll;

const btnSearch = document.getElementById('btnSearch');
if (btnSearch) {
    btnSearch.onclick = () => search(document.getElementById('q')?.value || '');
}

const btnAdd = document.getElementById('btnAdd');
if (btnAdd) btnAdd.onclick = addItem;

const btnRemove = document.getElementById('btnRemove');
if (btnRemove) btnRemove.onclick = removeItem;

// ==================== INITIALIZATION ====================
document.addEventListener('DOMContentLoaded', () => {
    console.log('✓ Inventory app initialized');
    initPageNavigation();
    listAll();
});
