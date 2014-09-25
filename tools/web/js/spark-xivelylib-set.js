//********************************************************************
// Web helper for XivelyLib for Spark
// http://github.com/davidegironi/spark-xivelylib
// Copyright (c) Davide Gironi, 2014 
//
// References:
//   https://github.com/jflasher/spark-helper
// 
// Released under GPLv3.
// Please refer to LICENSE file for licensing information.
//********************************************************************

$(document).ready(function() {
	var baseURL = "https://api.spark.io/v1/devices/";

	//load alert
	$("#info-alert").alert();
	$("#info-alert").affix();

	//load settings
	loadSettings();

	/**
	 * Load setting
	 */
	function loadSettings() {
		var coreID = localStorage.getItem("core-id");
		var apiToken = localStorage.getItem("api-token");
		if (coreID === undefined || coreID === '' || coreID === null || 
			apiToken === undefined || apiToken === '' || apiToken === null) {
			$("#settings-panel").removeClass("panel-default").addClass("panel-danger");
			return false;
		} else {
			$("#settings-panel").removeClass("panel-danger").addClass("panel-default");
			$("#core-id").val(localStorage.getItem("core-id"));
			$("#api-token").val(localStorage.getItem("api-token"));
			return true;
		}
	}

	/**
	 * Generic on method success
	 */
	function onMethodSuccess() {
		$("#info-alert").text("Success!").removeClass("alert-danger").addClass("alert-success");
		$("#info-alert").slideDown("slow");
		setTimeout(function() {
			$("#info-alert").slideUp("slow");
		}, 2000);
	}

	/**
	 * Generic on method failure
	 */
	function onMethodFailure() {
		$("#info-alert").text("Ruh roh!").removeClass("alert-success").addClass("alert-danger");
		$("#info-alert").slideDown("slow");
		setTimeout(function() {
			$("#info-alert").slideUp("slow");
		}, 2000);
	}

	/**
	 * Generic get variable method
	 */
	function getVariable(variable, callback) {
		var url = baseURL + $("#core-id").val() + "/" + variable + "?access_token=" + $("#api-token").val();
		$.getJSON(url, callback).fail(function(obj) {
			callback("0");
		})
	}

	/**
	 * Execute a method
	 */
	function doMethod(method, data, callback) {
		var url = baseURL + $("#core-id").val() + "/" + method;
		$.ajax({
			type: "POST",
			url: url,
			data: { access_token: $("#api-token").val(), args: data },
			success: function(obj) { callback("1") },
			dataType: "json"
		}).fail(function(obj) {
			callback("0");
		});
	}

	/**
	 * Check spark connection
	 */
	$("#spark-check").on("click", function () {
		getVariable("ping", function (res) {
			if(res.result != undefined && res.result == "1") {
				localStorage.setItem("core-id", $("#core-id").val());
				localStorage.setItem("api-token", $("#api-token").val());
				loadSettings();
				onMethodSuccess();
			} else {
				//$("#core-id").val("");
				//$("#api-token").val("");
				//localStorage.removeItem("core-id"); 
				//localStorage.removeItem("api-token"); 
				//loadSettings();
				onMethodFailure();
			}			
		});
	});

	/**
	 * Get xively paremeters
	 */
	$("#xively-get").on("click", function () {
		var error = false;
		getVariable("feedId", function (res) {
			if(res.result != undefined && res.result != "") {
				$("#feed-id").val(res.result);
			} else {
				$("#feed-id").val("");
				error = true;
			}			
		});
		getVariable("apiKey", function (res) {
			if(res.result != undefined && res.result != "") {
				$("#api-key").val(res.result);
			} else {
				$("#api-key").val("");
				error = true;
			}			
		});
		if(error) {
			onMethodFailure();
		} else {
			onMethodSuccess();
		}
	});

	/**
	 * Set xively paremeters
	 */
	$("#xively-set").on("click", function () {
		var error = false;
		doMethod("feedId", $("#feed-id").val(), function (res) {
			if(res != undefined && res == true) {
			} else {
				error = true;
			}
		});
		doMethod("apiKey", $("#api-key").val(), function (res) {
			if(res != undefined && res == true) {
			} else {
				error = true;
			}
		});
		if(error) {
			onMethodFailure();
		} else {
			onMethodSuccess();
		}
	});

});