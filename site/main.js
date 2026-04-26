const copyButtons = document.querySelectorAll(".copy-button");

for (const button of copyButtons) {
  button.addEventListener("click", async () => {
    const targetId = button.getAttribute("data-copy-target");
    const target = document.getElementById(targetId);
    if (!target) {
      return;
    }

    try {
      await navigator.clipboard.writeText(target.innerText.trim());
      const previous = button.textContent;
      button.textContent = "Скопировано";
      setTimeout(() => {
        button.textContent = previous;
      }, 1400);
    } catch (error) {
      const selection = window.getSelection();
      const range = document.createRange();
      range.selectNodeContents(target);
      selection.removeAllRanges();
      selection.addRange(range);
    }
  });
}
