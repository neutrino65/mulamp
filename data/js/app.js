const elements = {
  infoButton: document.querySelector("#infoButton"),
  closeModalButton: document.querySelector("#closeModalButton"),
  modalBackdrop: document.querySelector("#modalBackdrop"),
  modalTitle: document.querySelector("#modalTitle"),
  modalContent: document.querySelector("#modalContent"),
  manualForm: document.querySelector("#manualForm"),
  manualSsid: document.querySelector("#manualSsid"),
  manualPassword: document.querySelector("#manualPassword"),
  manualMessage: document.querySelector("#manualMessage"),
  refreshButton: document.querySelector("#refreshButton"),
  networkMeta: document.querySelector("#networkMeta"),
  networkList: document.querySelector("#networkList"),
  networkEmpty: document.querySelector("#networkEmpty"),
  networkCardTemplate: document.querySelector("#networkCardTemplate")
};

let lastFocusedElement = null;

document.addEventListener("DOMContentLoaded", () => {
  bindEvents();
  loadNetworks();
});

function bindEvents() {
  elements.infoButton.addEventListener("click", openDeviceInfo);
  elements.closeModalButton.addEventListener("click", closeModal);
  elements.refreshButton.addEventListener("click", loadNetworks);
  elements.manualForm.addEventListener("submit", handleManualSubmit);

  document.querySelector("#manualShowPassword").addEventListener("change", event => {
    elements.manualPassword.type = event.target.checked ? "text" : "password";
  });

  elements.modalBackdrop.addEventListener("click", event => {
    if (event.target === elements.modalBackdrop) {
      closeModal();
    }
  });

  document.addEventListener("keydown", event => {
    if (event.key === "Escape" && !elements.modalBackdrop.classList.contains("hidden")) {
      closeModal();
    }
  });
}

async function loadNetworks() {
  setRefreshing(true);
  elements.networkMeta.textContent = "Scanning nearby networks...";
  elements.networkEmpty.classList.add("hidden");

  try {
    const result = await scanNetworks();
    const networks = Array.isArray(result.networks) ? result.networks : [];
    renderNetworks(networks);
  } catch (error) {
    elements.networkMeta.textContent = "Could not scan networks.";
    elements.networkList.innerHTML = "";
    showEmptyState("Scan failed. Try again.");
    console.error(error);
  } finally {
    setRefreshing(false);
  }
}

function renderNetworks(networks) {
  elements.networkList.innerHTML = "";
  elements.networkMeta.textContent =
    `${networks.length} network${networks.length === 1 ? "" : "s"} · Sorted by signal`;

  if (!networks.length) {
    showEmptyState("No networks found.");
    return;
  }

  networks
    .sort((a, b) => Number(b.rssi) - Number(a.rssi))
    .forEach(network => {
      const card = elements.networkCardTemplate.content.cloneNode(true);
      const button = card.querySelector(".network-card");

      card.querySelector(".network-name").textContent = network.ssid || "(Hidden network)";
      card.querySelector(".network-security").textContent =
        `${network.security || "Unknown security"} · Channel ${network.channel ?? "—"}`;
      card.querySelector(".network-rssi").textContent =
        `${network.rssi ?? "—"} dBm`;

      button.addEventListener("click", () => openNetworkModal(network));
      elements.networkList.appendChild(card);
    });
}

function showEmptyState(message) {
  elements.networkEmpty.classList.remove("hidden");
  elements.networkEmpty.querySelector("strong").textContent = message;
  elements.networkEmpty.querySelector("span").textContent = "Try scanning again.";
}

function setRefreshing(isRefreshing) {
  elements.refreshButton.disabled = isRefreshing;
  elements.refreshButton.textContent = isRefreshing ? "…" : "↻";
}

async function openDeviceInfo() {
  lastFocusedElement = document.activeElement;
  openModal("Device information", "<p>Loading device information...</p>");

  try {
    const device = await getDeviceInfo();

    elements.modalContent.innerHTML = `
      <div class="info-grid">
        ${infoRow("Microcontroller", device.device)}
        ${infoRow("Device owner", device.owner)}
        ${infoRow("Initialization date", device.initialization_date)}
        ${infoRow("Firmware version", device.firmware_version || "Not provided")}
        <div class="info-row internet-status-row">
          <div class="status-field-content">
            <span class="info-label">ESP INTERNET STATUS</span>
            <span class="info-value" id="espInternetStatus">Not checked</span>
            <span class="status-ssid" id="espConnectedSsid"></span>
          </div>
          <button class="check-status-button" id="checkInternetStatus" type="button">Check</button>
        </div>
      </div>
    `;

    document.querySelector("#checkInternetStatus").addEventListener("click", checkEspInternetStatus);
  } catch (error) {
    elements.modalContent.innerHTML = `<p class="form-message error">Unable to load device information.</p>`;
    console.error(error);
  }
}

async function checkEspInternetStatus() {
  const button = document.querySelector("#checkInternetStatus");
  const status = document.querySelector("#espInternetStatus");
  const ssid = document.querySelector("#espConnectedSsid");

  if (!button || !status || !ssid) return;

  button.disabled = true;
  button.textContent = "Checking...";
  status.textContent = "Checking...";
  status.className = "info-value";
  ssid.textContent = "";

  try {
    const result = await getInternetStatus();

    if (result.connected) {
      status.textContent = "Connected";
      status.className = "info-value status-connected";
      ssid.textContent = `SSID: ${result.ssid || "Unknown"}`;
    } else {
      status.textContent = "Not connected";
      status.className = "info-value status-disconnected";
      ssid.textContent = "No Wi-Fi connected";
    }
  } catch (error) {
    status.textContent = "Unable to check";
    status.className = "info-value status-disconnected";
    ssid.textContent = "ESP8266 did not respond.";
    console.error(error);
  } finally {
    button.disabled = false;
    button.textContent = "Check";
  }
}

function infoRow(label, value) {
  return `
    <div class="info-row">
      <span class="info-label">${escapeHtml(label)}</span>
      <span class="info-value">${escapeHtml(value ?? "—")}</span>
    </div>
  `;
}

function openNetworkModal(network) {
  lastFocusedElement = document.activeElement;

  openModal(
    "Connect to Wi-Fi",
    `
      <form class="modal-form" id="networkForm">
        <label for="selectedSsid">SSID</label>
        <input id="selectedSsid" type="text" value="${escapeHtml(network.ssid || "")}" readonly />

        <label for="selectedPassword">Password</label>
        <div class="password-field">
          <input id="selectedPassword" type="password" placeholder="Enter password" required />
          <label class="show-password">
            <input id="selectedShowPassword" type="checkbox" />
            <span>Show password</span>
          </label>
        </div>

        <button class="primary-button" type="submit">
          <span>Submit</span>
          <span aria-hidden="true">↗</span>
        </button>
        <p class="form-message" id="networkMessage" role="status"></p>
      </form>
    `
  );

  const form = document.querySelector("#networkForm");
  const passwordInput = document.querySelector("#selectedPassword");
  const showPassword = document.querySelector("#selectedShowPassword");

  showPassword.addEventListener("change", event => {
    passwordInput.type = event.target.checked ? "text" : "password";
  });

  form.addEventListener("submit", async event => {
    event.preventDefault();

    const message = document.querySelector("#networkMessage");
    const submitButton = form.querySelector("button[type='submit']");
    const password = passwordInput.value.trim();

    if (!password) {
      setMessage(message, "Please enter the Wi-Fi password.", "error");
      return;
    }

    submitButton.disabled = true;
    submitButton.querySelector("span").textContent = "Sending...";
    setMessage(message, "", "");

    try {
      const result = await submitWifiCredentials(network.ssid, password);
      setMessage(
        message,
        result.message || (result.success ? "Credentials submitted." : "Connection failed."),
        result.success ? "success" : "error"
      );

      if (result.success && !USE_MOCK_API) {
        passwordInput.value = "";
      }
    } catch (error) {
      setMessage(message, "Please check wifi creds again.", "error");
      console.error(error);
    } finally {
      submitButton.disabled = false;
      submitButton.querySelector("span").textContent = "Submit";
    }
  });

  passwordInput.focus();
}

async function handleManualSubmit(event) {
  event.preventDefault();

  const ssid = elements.manualSsid.value.trim();
  const password = elements.manualPassword.value.trim();

  if (!ssid || !password) {
    setMessage(elements.manualMessage, "SSID and password are required.", "error");
    return;
  }

  const submitButton = elements.manualForm.querySelector("button[type='submit']");
  submitButton.disabled = true;
  submitButton.querySelector("span").textContent = "Sending...";
  setMessage(elements.manualMessage, "", "");

  try {
    const result = await submitWifiCredentials(ssid, password);
    setMessage(
      elements.manualMessage,
      result.message || (result.success ? "Credentials submitted." : "Connection failed."),
      result.success ? "success" : "error"
    );
  } catch (error) {
    setMessage(elements.manualMessage, "Could not reach the ESP8266.", "error");
    console.error(error);
  } finally {
    submitButton.disabled = false;
    submitButton.querySelector("span").textContent = "Submit";
  }
}

function setMessage(element, message, type) {
  element.textContent = message;
  element.className = `form-message ${type || ""}`;
}

function openModal(title, content) {
  elements.modalTitle.textContent = title;
  elements.modalContent.innerHTML = content;
  elements.modalBackdrop.classList.remove("hidden");
  document.body.style.overflow = "hidden";
}

function closeModal() {
  elements.modalBackdrop.classList.add("hidden");
  document.body.style.overflow = "";

  if (lastFocusedElement && typeof lastFocusedElement.focus === "function") {
    lastFocusedElement.focus();
  }
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}
