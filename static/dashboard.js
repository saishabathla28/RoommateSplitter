(function dashboardClient() {
  const content = document.querySelector("#content");
  const dialog = document.querySelector("#dialog");
  const form = document.querySelector("#action-form");
  const error = document.querySelector("#error");
  const formError = document.querySelector("#form-error");
  const submit = document.querySelector("#submit");
  const currency = new Intl.NumberFormat("en-IN", { minimumFractionDigits: 2, maximumFractionDigits: 2 });
  const money = amount => `₹${currency.format(amount / 100)}`;
  const escape = value => String(value ?? "").replace(/[&<>"']/g, c =>
    ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[c]);
  const icons = {
    Rent: "⌂", Electricity: "ϟ", Water: "◌", "Wi-Fi": "≋", "Cooking gas": "♨",
    Groceries: "▧", "Cleaning supplies": "✧", "Repairs and maintenance": "+", Other: "·",
  };
  const pages = { overview: "Overview", expenses: "Expenses", roommates: "Roommates", payments: "Payments" };
  let state, action, actionDetails, toastTimer;
  let filter = "";

  function localDate() {
    const now = new Date();
    return new Date(now - now.getTimezoneOffset() * 60000).toISOString().slice(0, 10);
  }

  async function request(path, data) {
    const response = await fetch(`/api/${path}`, data === undefined ? {} : {
      method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(data),
    });
    const result = await response.json();
    if (!response.ok) throw new Error(result.error || "Something went wrong.");
    return result;
  }

  async function load() {
    state = await request("dashboard");
    error.hidden = true;
    render();
  }

  const name = id => escape(state.roommates.find(person => person.id === id)?.name ?? "Roommate");
  const active = () => state.roommates.filter(person => person.active);
  const heading = (title, link = "") => `<div class="card-heading"><h2>${title}</h2>${link}</div>`;
  const empty = message => `<p class="muted">${message}</p>`;

  function expenseRow(expense, expandable = false) {
    const contents = `<div class="item"><span class="badge" aria-hidden="true">${icons[expense.category] || "·"}</span>
      <div><strong>${escape(expense.category)}</strong><p>${expense.note && expandable ?
        `${escape(expense.note)} · ` : ""}Paid by ${name(expense.paid_by)}</p></div></div>
      <span class="amount">${money(expense.amount)}</span>`;
    if (!expandable) return `<div class="row">${contents}</div>`;
    return `<details><summary><div class="row">${contents}</div></summary>
      <div class="row"><span class="muted">Expense date</span><span>${escape(expense.expense_date)}</span></div>
      ${expense.shares.map(share => `<div class="row"><span>${name(share.roommate_id)}</span>
        <span>${money(share.amount)}</span></div>`).join("")}</details>`;
  }

  function settlementRows() {
    return state.settlements.map(transfer => `<div class="row">
      <div><strong>${name(transfer.sender)}</strong><p>owes ${name(transfer.receiver)}</p></div>
      <button class="secondary" type="button" data-open="payment" data-sender="${transfer.sender}"
        data-receiver="${transfer.receiver}" data-amount="${transfer.amount}"
        aria-label="Record payment of ${money(transfer.amount)} from ${name(transfer.sender)}">${money(transfer.amount)}</button>
      </div>`).join("") || empty("Everyone is settled up.");
  }

  function overview() {
    const month = localDate().slice(0, 7);
    const monthExpenses = state.expenses.filter(expense => expense.expense_date.startsWith(month));
    const total = monthExpenses.reduce((sum, expense) => sum + expense.amount, 0);
    const outstanding = state.settlements.reduce((sum, payment) => sum + payment.amount, 0);
    const categories = new Map();
    for (const expense of monthExpenses) {
      categories.set(expense.category, (categories.get(expense.category) || 0) + expense.amount);
    }
    const breakdown = [...categories.entries()].sort((a, b) => b[1] - a[1]).map(([category, amount]) => `
      <div class="breakdown"><div class="row"><span>${escape(category)}</span><strong>${money(amount)}</strong></div>
      <div class="bar" aria-hidden="true"><span style="width:${amount / total * 100}%"></span></div></div>`).join("");
    return `<section class="hero"><h1>Apartment overview</h1><p>Shared expenses and balances in one place.</p></section>
      <div class="metrics">
        <div class="metric"><span>Spent this month</span><strong>${money(total)}</strong></div>
        <div class="metric"><span>Outstanding</span><strong>${money(outstanding)}</strong></div>
        <div class="metric"><span>Current roommates</span><strong>${active().length}</strong></div>
      </div>
      <div class="columns"><section class="card">
      ${heading("Recent expenses", '<a href="#expenses">View all ↗</a>')}
      ${state.expenses.slice(0, 5).map(item => expenseRow(item)).join("") || empty("No expenses yet.")}
      </section><div class="stack"><section class="card">${heading("Who owes whom")}${settlementRows()}</section>
      <section class="card">${heading("Spending by category")}${breakdown || empty("No expenses this month.")}</section></div></div>`;
  }

  function expenses() {
    const options = state.categories.map(category => `<option${filter === category ? " selected" : ""}>${escape(category)}</option>`);
    const list = state.expenses.filter(expense => !filter || expense.category === filter);
    return `<div class="page-heading"><h1>Expenses</h1><p>Open an expense to see how it was split.</p></div>
      <section class="card">${heading("Expense history", `<div class="filters"><select id="category-filter" aria-label="Filter by category">
        <option value="">All categories</option>${options.join("")}</select></div>`)}
      ${list.map(expense => expenseRow(expense, true)).join("") || empty("No expenses in this category.")}</section>`;
  }

  function roommates() {
    return `<div class="page-heading"><h1>Roommates</h1></div><section class="card">
      ${heading("Current roommates", '<button data-open="roommate">+ Add roommate</button>')}
      ${active().map(person => `<div class="row"><div class="item">
        <span class="avatar" aria-hidden="true">${escape(person.name.split(/\s+/).slice(0, 2).map(part => part[0]).join("").toUpperCase())}</span>
        <div><strong>${escape(person.name)}</strong><p>Room ${escape(person.room)}</p></div></div>
        <button class="danger" data-open="remove" data-roommate="${person.id}">Remove</button></div>`).join("") ||
        empty("Add someone to start sharing expenses.")}</section>`;
  }

  function payments() {
    return `<div class="page-heading"><h1>Payments</h1><p>Record a payment after a roommate has sent the money.</p></div>
      <div class="stack"><section class="card">${heading("Who owes whom", state.settlements.length ?
        '<button data-open="payment">+ Record payment</button>' : "")}${settlementRows()}</section>
      <section class="card">${heading("Payment history")}
      ${state.payments.map(payment => `<div class="row"><div><strong>${name(payment.sender)} paid ${name(payment.receiver)}</strong>
        <p>${escape(payment.payment_date)}</p></div><span class="amount">${money(payment.amount)}</span></div>`).join("") ||
        empty("No payments recorded yet.")}</section></div>`;
  }

  function render() {
    if (!state) return;
    const hash = location.hash.slice(1);
    const page = Object.hasOwn(pages, hash) ? hash : "overview";
    document.querySelector("#breadcrumb").textContent = `Apartment / ${pages[page]}`;
    document.querySelectorAll("nav a").forEach(link => {
      if (link.hash === `#${page}`) link.setAttribute("aria-current", "page");
      else link.removeAttribute("aria-current");
    });
    content.innerHTML = { overview, expenses, roommates, payments }[page]();
    document.querySelector("#category-filter")?.addEventListener("change", event => {
      filter = event.target.value;
      render();
      document.querySelector("#category-filter")?.focus();
    });
  }

  const option = (value, label, selected) =>
    `<option value="${escape(value)}"${String(value) === selected ? " selected" : ""}>${label}</option>`;
  const select = (label, key, options) => `<label>${label}<select name="${key}" required>${options.join("")}</select></label>`;
  const amountField = amount => `<label>Amount (₹)<input name="amount" inputmode="decimal" placeholder="0.00" maxlength="12"
    value="${amount ? (Number(amount) / 100).toFixed(2) : ""}" required></label>`;
  const dateField = `<label>Date<input name="date" type="date" required></label>`;

  function openForm(button) {
    if (!state) return;
    action = button.dataset.open;
    const fields = document.querySelector("#fields");
    let title, html, label;
    actionDetails = {};
    if (action === "expense") {
      title = "Add expense"; label = "Add expense";
      if (!active().length) {
        notify("Add a roommate before adding an expense.");
        return;
      }
      html = select("Category", "category", state.categories.map(category => option(category, escape(category)))) +
        select("Who paid?", "paid_by", active().map(person => option(person.id, name(person.id)))) +
        amountField() + dateField + '<label>Note (optional)<input name="note" maxlength="150"></label>';
    } else if (action === "roommate") {
      title = "Add roommate"; label = "Add roommate";
      html = '<label>Name<input name="name" maxlength="80" required></label>' +
        '<label>Room<input name="room" maxlength="30" required></label>';
    } else if (action === "remove") {
      title = "Remove roommate"; label = "Remove roommate";
      actionDetails.roommate = Number(button.dataset.roommate);
      html = `<p>Remove <strong>${name(actionDetails.roommate)}</strong> from the current household?
        Their past expenses and balance stay on record.</p>`;
    } else {
      title = "Record payment"; label = "Record payment";
      const debtors = state.roommates.filter(person => person.balance < 0);
      const creditors = state.roommates.filter(person => person.balance > 0);
      html = select("Who sent the money?", "sender", debtors.map(person =>
        option(person.id, name(person.id), button.dataset.sender))) +
        select("Who received it?", "receiver", creditors.map(person =>
          option(person.id, name(person.id), button.dataset.receiver))) +
        amountField(button.dataset.amount) + dateField;
    }
    document.querySelector("#dialog-title").textContent = title;
    fields.innerHTML = html;
    if (form.elements.date) form.elements.date.value = localDate();
    formError.hidden = true;
    submit.textContent = label;
    dialog.showModal();
  }

  function notify(message) {
    const notice = document.querySelector("#notice");
    notice.textContent = message;
    notice.hidden = false;
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => { notice.hidden = true; }, 3500);
  }

  document.addEventListener("click", event => {
    const open = event.target.closest("[data-open]");
    if (open) openForm(open);
    if (event.target.closest("[data-close]")) dialog.close();
  });
  dialog.addEventListener("cancel", event => {
    if (submit.disabled) event.preventDefault();
  });
  form.addEventListener("submit", async event => {
    event.preventDefault();
    const data = { ...Object.fromEntries(new FormData(form)), ...actionDetails };
    for (const key of ["paid_by", "sender", "receiver"]) {
      if (key in data) data[key] = Number(data[key]);
    }
    const current = action;
    submit.disabled = true;
    const closeButtons = Array.from(form.querySelectorAll("[data-close]"));
    closeButtons.forEach(button => { button.disabled = true; });
    formError.hidden = true;
    try {
      await request({ expense: "expenses", roommate: "roommates", remove: "roommates/remove", payment: "payments" }[current], data);
      dialog.close();
      notify({ expense: "Expense added.", roommate: "Roommate added.", remove: "Roommate removed.", payment: "Payment recorded." }[current]);
      await load();
    } catch (failure) {
      const target = dialog.open ? formError : error;
      target.textContent = failure.message;
      target.hidden = false;
    } finally {
      submit.disabled = false;
      closeButtons.forEach(button => { button.disabled = false; });
    }
  });
  window.addEventListener("hashchange", render);
  load().catch(failure => {
    content.textContent = "";
    error.textContent = failure.message;
    error.hidden = false;
  });
})();
