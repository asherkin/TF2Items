function setupHover() {
	$("a.item").unbind('mouseover');
	$("a.item").unbind('mouseout');
	$("a.item").unbind('mousedown');
	$("a.item").unbind('mouseup');
	$("a.item").mouseover(function() {
		$(this).parent().removeClass("cellSelected");
		$(this).parent().addClass("cellHover");
	});
	$("a.item").mouseout(function() {
		$(this).parent().removeClass("cellHover");
	});
	$("a.item").mousedown(function() {
		$(this).parent().removeClass("cellHover");
		$(this).parent().addClass("cellSelected");
	});
	$("a.item").mouseup(function() {
		$(this).parent().removeClass("cellSelected");
		$(this).parent().addClass("cellHover");
	});
}
$(setupHover);
